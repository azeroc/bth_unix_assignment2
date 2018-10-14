#include <net_thread.h>
#include <common.h>
#include <http.h>

// Starts thread-based web listening, requests get split off in their own separate threads
// Returns exit-error
int thread_listen(config_t* conf)
{
    pthread_t thread_id;
    thread_data_t* td;
    socklen_t socklen; // Sizeof(sockaddr_in)
    int listen_sock, client_sock;
    struct sockaddr_in server, client;
    char server_ip_str[INET_ADDRSTRLEN];
    char client_ip_str[INET_ADDRSTRLEN];

    // Create listening socket
    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // We want TCP protocol for HTTP
    if (listen_sock == -1) {
        printf("[ERROR] [thread_listen] Failed to create listening sock, error: %s\n", strerror(errno));
        return 1;
    }

    // Setup listening socket's sockeaddr_in
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; // We will listen on all interfaces
    server.sin_port = htons(conf->port);
    inet_ntop( AF_INET, &server.sin_addr, server_ip_str, INET_ADDRSTRLEN );

    // Bind listening socket
    if ( bind(listen_sock, (struct sockaddr*)&server, sizeof(server)) < 0 ) {
        printf("[ERROR] [thread_listen] Failed to bind listening sock [%s:%d], error: %s\n", server_ip_str, ntohs(server.sin_port), strerror(errno));
        return 1;
    }

    // Turn on listening mode (can queue up to SOMAXCONN connections for listening)
    listen(listen_sock, SOMAXCONN);

    // While we can accept new socket connections without issue (non-zero client_sock), continue listen/accept loop
    socklen = (socklen_t)sizeof(struct sockaddr_in);
    printf("[INFO] [thread_listen] Server [%s:%d] listening for connections...\n", server_ip_str, ntohs(server.sin_port));

    while ( (client_sock = accept(listen_sock, (struct sockaddr*)&client, &socklen)) ) {
        // For debug logging
        inet_ntop( AF_INET, &client.sin_addr, client_ip_str, INET_ADDRSTRLEN );
        printf("[INFO] [thread_listen] Accepted connection: [%s:%d] -> [%s:%d]\n", client_ip_str, ntohs(client.sin_port), server_ip_str, ntohs(server.sin_port));

        // Allocate and setup thread data (it will be freed by the thread)
        td = (thread_data_t*)malloc(sizeof(thread_data_t));
        td->socket_id = client_sock;
        td->conf = conf;

        // Create request handling thread and handoff newly allocated thread_data object
        if ( pthread_create(&thread_id, NULL, thread_handle_request, (void*) td) < 0 ) {
            printf("[ERROR] [thread_listen] Failed to pthread_create request handler thread, error: %s\n", strerror(errno));
            return 1;
        }
    }

    if (client_sock < 0) {
        printf("[ERROR] [thread_listen] Failed to accept client connection, error: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

// Request processing function for POSIX thread
void* thread_handle_request(void* thread_data)
{
    // Stack variable declarations
    http_request_t request;
    int terminated = 0; // Flag for message termination when encountering \r\n\r\n or \n\n
    int too_big = 0; // Flag for message being too big (no termination and exceeded buffer size)
    int read_bytes;
    char byte1 = '\0', byte2 = '\0', byte3 = '\0', byte4 = '\0'; // For determining HTTP request termination
    int sbuffer_itr = 0;
    int message_itr = 0;
    int response_send_ec = 0; // Response sending exit code
    char socket_buffer[CONF_SOCK_BUFSIZE+1];
    char message_buffer[CONF_REQ_BUFSIZE+1];
    memset(message_buffer, 0, CONF_REQ_BUFSIZE+1); // Make sure message buffer is 100% clear/clean

    // Cast void* back to proper type
    thread_data_t* td = (thread_data_t*) thread_data;

    // Begin read-loop
    // Example request from client:
    /*
        GET /index.html HTTP/1.0\r\n
        Host: www.example.com\r\n
        \r\n
    */
    while ( (read_bytes = recv(td->socket_id, socket_buffer, CONF_SOCK_BUFSIZE, 0)) > 0 ) {
        // Delimiter at the end of received bytes
        socket_buffer[read_bytes] = '\0';
        sbuffer_itr = 0;

        // Go through bytes, check for termination, otherwise write to message buffer
        while (socket_buffer[sbuffer_itr] != '\0') {
            // Message too big (return 400 - Bad Request)
            if (message_itr > CONF_REQ_BUFSIZE) {
                too_big = 1;
                break;
            }

            // This shifts and saves last 4 bytes during iteration
            byte4 = byte3; byte3 = byte2; byte2 = byte1;
            byte1 = socket_buffer[sbuffer_itr];

            // Write to message buffer
            message_buffer[message_itr] = socket_buffer[sbuffer_itr];

            // Termination signal: \r\n\r\n (correct) or \n\n (tolerant)
            if ((byte4 == '\r' && byte3 == '\n' && byte2 == '\r' && byte1 == '\n') || (byte2 == '\n' && byte1 == '\n')) {
                terminated = 1;
                break;
            }

            sbuffer_itr++;
            message_itr++;
        }

        // If request was too long (no termination detected), return 400 - Bad Request
        if (too_big == 1) {
            response_send_ec = send_http_error_response(td->socket_id, td->conf, NULL, HTTP_STATUS_BADREQUEST);
            break;
        }

        if (terminated == 1) {
            printf("[INFO] [socket: %d] Received %ld content-length request payload\n", td->socket_id, strlen(message_buffer));
            if (parse_http_request(message_buffer, &request) == 0) { 
                response_send_ec = send_http_response(td->socket_id, td->conf, &request);
            } else {
                response_send_ec = send_http_error_response(td->socket_id, td->conf, NULL, HTTP_STATUS_BADREQUEST);
            }
            break;
        }
    }

    // Check if something bad happened to connection
    if (read_bytes < 0) {
        printf("[ERROR] [socket: %d] Connection issue, error: %s\n", td->socket_id, strerror(errno));
    }

    // Check if response was formed and sent successfully
    if (response_send_ec != 0) {
        printf("[ERROR] [socket: %d] Failed to send HTTP response, error: %s\n", td->socket_id, strerror(errno));
    }

    // Cleanup and exit
    close(td->socket_id); // Close the socket/connection
    free(td); // Free thread_data object memory from heap
    //pthread_exit(NULL); // Exit this pthread, causes issues when running with chroot
    return NULL;
}