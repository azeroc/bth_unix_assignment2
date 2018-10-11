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
        // TODO: Logging
        printf("[main->thread_listen] [ERROR] Failed to create listening sock, error: %s\n", strerror(errno));
        return 1;
    }

    // Setup listening socket's sockeaddr_in
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; // We will listen on all interfaces
    server.sin_port = htons(conf->port);
    inet_ntop( AF_INET, &server.sin_addr, server_ip_str, INET_ADDRSTRLEN );

    // Bind listening socket
    if ( bind(listen_sock, (struct sockaddr*)&server, sizeof(server)) < 0 ) {
        printf("[main->thread_listen] [ERROR] Failed to bind listening sock [%s:%d], error: %s\n", server_ip_str, ntohs(server.sin_port), strerror(errno));
        return 1;
    }

    // Turn on listening mode (can queue up to SOMAXCONN connections for listening)
    listen(listen_sock, SOMAXCONN);

    // While we can accept new socket connections without issue (non-zero client_sock), continue listen/accept loop
    socklen = (socklen_t)sizeof(struct sockaddr_in);
    printf("[main->thread_listen] [INFO] Server [%s:%d] listening for connections...\n", server_ip_str, ntohs(server.sin_port));

    while ( (client_sock = accept(listen_sock, (struct sockaddr*)&client, &socklen)) ) {
        // For debug logging
        inet_ntop( AF_INET, &client.sin_addr, client_ip_str, INET_ADDRSTRLEN );
        printf("[main->thread_listen] [DEBUG] Accepted connection: [%s:%d] -> [%s:%d]\n", client_ip_str, ntohs(client.sin_port), server_ip_str, ntohs(server.sin_port));

        // Allocate and setup thread data (it will be freed by the thread)
        td = (thread_data_t*)malloc(sizeof(thread_data_t));
        td->socket_id = client_sock;
        td->conf = conf;

        // Create request handling thread and handoff newly allocated thread_data object
        if ( pthread_create(&thread_id, NULL, thread_handle_request, (void*) td) < 0 ) {
            printf("[main->thread_listen] [ERROR] Failed to pthread_create request handler thread, error: %s\n", strerror(errno));
            return 1;
        }
    }

    if (client_sock < 0) {
        printf("[main->thread_listen] [ERROR] Failed to accept client connection, error: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

// Request processing function for POSIX thread
void* thread_handle_request(void* thread_data)
{
    // Stack variable declarations
    //http_request_t request;
    //http_request_t response;
    int terminated = 0; // Flag for message termination when encountering \r\n\r\n or \n\n
    int too_long = 0; // Flag for message being too long (no termination)
    int read_bytes;
    char byte1 = '\0', byte2 = '\0', byte3 = '\0', byte4 = '\0'; // For determining HTTP request termination
    int sbuffer_itr = 0;
    int message_itr = 0;

    // Cast void* back to proper type
    thread_data_t* td = (thread_data_t*) thread_data;
    printf("[socket_id: %d] [thread_handle_request] [DEBUG] Created\n", td->socket_id);

    // Allocate socket_buffer
    // Note: +1 for '\0' delimiter, in case we receive exactly 8kB big message
    char* socket_buffer = (char*)malloc(td->conf->sock_bufsize+1);
    char* message_buffer = (char*)malloc(td->conf->req_bufsize+1);
    memset(message_buffer, 0, td->conf->req_bufsize); // Make sure message buffer is 100% clear/clean

    // Begin read-loop
    // Example request from client:
    /*
        GET /index.html HTTP/1.0\r\n
        Host: www.example.com\r\n
        \r\n
    */
    while ( (read_bytes = recv(td->socket_id, socket_buffer, td->conf->sock_bufsize, 0)) > 0 ) {
        // Delimiter at the end of received bytes
        socket_buffer[read_bytes] = '\0';
        sbuffer_itr = 0;

        // Go through bytes, check for termination, otherwise write to message buffer
        while (socket_buffer[sbuffer_itr] != '\0') {
            // Message too big (return 400 - Bad Request)
            if (message_itr > td->conf->req_bufsize) {
                too_long = 1;
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
        if (too_long == 1) {
            // TODO: Fetch from file controller
            write(td->socket_id, "HTTP/1.0 400 Bad Request\r\n\r\n", strlen("HTTP/1.0 400 Bad Request\r\n\r\n"));
            break;
        }

        if (terminated == 1) {
            // TODO: Parse HTTP request and return correct response
            write(td->socket_id, message_buffer, strlen(message_buffer));
            break;
        }
    }

    // Cleanup and exit
    printf("[socket_id: %d] [thread_handle_request] [DEBUG] Clean-up and closing ...\n", td->socket_id);
    close(td->socket_id); // Close the socket/connection
    free(socket_buffer); // Free buffer memory from heap
    free(message_buffer); // Free buffer memory from heap
    free(td); // Free thread_data object memory from heap
    pthread_exit(NULL); // Exit this pthread
}