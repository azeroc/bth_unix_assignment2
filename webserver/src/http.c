#include <http.h>

// Status code enum to string
const char* http_status_str(http_status_t status)
{
    switch (status) {
        case HTTP_STATUS_OK:
            return "OK";
        case HTTP_STATUS_BADREQUEST:
            return "Bad Request";
        case HTTP_STATUS_FORBIDDEN:
            return "Forbidden";
        case HTTP_STATUS_NOTFOUND:
            return "Not Found";
        case HTTP_STATUS_INTERNALSERVERERROR:
            return "Internal Server Error";
        case HTTP_STATUS_NOTIMPLEMENTED:
            return "Not Implemented";
        default:
            return "Unknown";
    }
}

// Get Content-Type from document/resource file extension
const char* doc_content_type(const char* docpath)
{
    const char* lslash = strrchr(docpath, '/');
    const char* ext = strrchr(docpath, '.');

    // Default to TEXT/HTML for broken paths or missing extensions
    if (ext == NULL || (lslash != NULL && ext < lslash)) { 
        return CONTENT_TEXT_HTML;
    }

    if      (!strcmp(ext, ".txt" )) { return CONTENT_TEXT_PLAIN; }
    else if (!strcmp(ext, ".htm" )) { return CONTENT_TEXT_HTML; }
    else if (!strcmp(ext, ".html")) { return CONTENT_TEXT_HTML; }
    else if (!strcmp(ext, ".css" )) { return CONTENT_TEXT_CSS; }
    else if (!strcmp(ext, ".ico" )) { return CONTENT_IMG_ICO; }
    else if (!strcmp(ext, ".jpeg")) { return CONTENT_IMG_JPEG; }
    else if (!strcmp(ext, ".jpg" )) { return CONTENT_IMG_JPEG; }
    else if (!strcmp(ext, ".png" )) { return CONTENT_IMG_PNG; }
    else if (!strcmp(ext, ".gif" )) { return CONTENT_IMG_GIF; }
    else if (!strcmp(ext, ".js"  )) { return CONTENT_APP_JS; }
    else if (!strcmp(ext, ".xml" )) { return CONTENT_APP_XML; }
    else if (!strcmp(ext, ".bin" )) { return CONTENT_APP_OCTET_STREAM; }
    else                            { return CONTENT_DEFAULT; }
}

// Helper function to determine if x is a hex character
int is_hex(char x)
{
	return	(x >= '0' && x <= '9') || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F');
}

// Decode URI in given uri buffer (modifying it)
// Return 0 for successful decode, 1 for bad decode
int uri_decode(const char *src, char *dest)
{
	char *o; // Output writing pointer
    const char *p = src; // Iterating pointer for src
	const char *end = src + strlen(src);
	int c;
 
	for (o = dest; p <= end; o++) {
		c = *p++;

        // Replace + with space
		if (c == '+') { c = ' '; }
        // Replace percent-hex-encoded values with ASCII characters
		else if ( c == '%' && (!is_hex(*p++)	|| !is_hex(*p++) || !sscanf(p - 2, "%2x", &c)) ) {
            return 1;
        }

		if (dest) { *o = c; }
	}
 
	return 0;
}


// Helper function - parse Request-Line byte-by-byte
// Example Request-Line (without quotes): "HEAD /index.html HTTP/1.0\r\n"
// Format spec: https://www.w3.org/Protocols/HTTP/1.0/spec.html#Request-Line
// Return 0 on successful parse, 1 on bad parse
int parse_request_line(char* message_buf, http_request_t* http_request)
{
    char c = 0, cn = 0; // current char, next char
    char* token_ptr = message_buf;
    int msg_itr = 0;

    // Initialize http_request
    http_request->message_buf = message_buf;
    http_request->method = NULL;
    http_request->uri = NULL;
    http_request->version = NULL;
    http_request->header_fields = NULL;

    // Get current and next character from message buffer
    c = message_buf[msg_itr];
    cn = message_buf[msg_itr+1];

    // Check if Request-Line begins with Method (instead of spaces or weird characters)
    if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z')) {
        return 1;
    }

    // Go through message buffer until first newline (whether \r\n or \n)
    // Replace spaces with \0 to divide message buffer into individual strings
    // Make http_request method, uri and version point to these isolated strings
    // (first one being method, second one being uri and etc.)
    while (c != '\n' && c != '\r') {
        // If we encounter illegal whitespace characters 
        if (c == '\t' || c == '\v' || c == '\f') {
            return 1;
        }

        // If we see that our token is about to end (next character is space or possible end-of-line), 
        // then copy current token pointer value to first non-NULL Request-Line pointer variable (method, uri, version)
        if (c != ' ' && (cn == ' ' || cn == '\r' || cn == '\n')) {
            if      (http_request->method == NULL)  { http_request->method = token_ptr; }
            else if (http_request->uri == NULL)     { http_request->uri = token_ptr; }
            else if (http_request->version == NULL) { http_request->version = token_ptr; }
            else { // If we get 4 or more tokens, then Request-Line has invalid format - bad parse
                return 1;
            }
        }

        // Zero out spaces in Request-Line, which splits line into its own string tokens
        // For example - "HEAD /index.html HTTP/1.0\r\n" becomes
        // "HEAD", "/index.html", "HTTP/1.0" with \0 between em
        if (c == ' ') { 
            message_buf[msg_itr] = '\0';
        }

        // If we see that next character begins a new token, move token pointer to it's beginning
        if (c == ' ' && cn != ' ' && cn != '\r' && cn != '\n') {
            token_ptr = &message_buf[msg_itr+1];
        }

        msg_itr++;
        c = message_buf[msg_itr];
        cn = message_buf[msg_itr+1];        
    }

    // Make sure all 3 Request-Line fields are assigned
    if (http_request->method == NULL || http_request->uri == NULL || http_request->version == NULL) {
        return 1;
    }

    // Zero-out Request-Line end-line and set header_fields pointer to point to characters after
    if (c == '\r' && cn == '\n') { // CRLF case
        message_buf[msg_itr] = '\0';
        message_buf[msg_itr+1] = '\0';
        http_request->header_fields = &message_buf[msg_itr+2];
    } else { // LF case
        message_buf[msg_itr] = '\0';
        http_request->header_fields = &message_buf[msg_itr+1];
    }

    return 0;
}

// Parse and potentially fix doc_path from URI
// Returns 0 if parsing was successful, 1 if not (parsed doc_path was too long for PATH_MAX)
int parse_doc_path_uri(char* doc_path, const char* uri, int len)
{
    // End point will always be end of uri or until first question mark
    const char* p_end = strchr(uri, '?');
    if (p_end == NULL) {
        p_end = uri + strlen(uri);
    }
    
    // Document path will start at the beginning if it is relative path, but we have to check if it is absolute path (e.g. "http://www.google.com/index.html")
    const char* p_start;
    if (strncmp(uri, "http://", 7) == 0) { // If we encounter "http://www.google.com/index.html"
        p_start = strchr(uri + 7, '/'); // After http:// schema, go to first slash which should get us back to relative path
        p_start = (p_start == NULL) ? uri + 7 : p_start; // If it doesn't exist for some reason, just start after schema
    } else if (strncmp(uri, "https://", 8) == 0) { // If we encounter "https://www.google.com/index.html"
        p_start = strchr(uri + 8, '/'); // After https:// schema, go to first slash which should get us back to relative path
        p_start = (p_start == NULL) ? uri + 7 : p_start; // If it doesn't exist for some reason, just start after schema
    } else if (uri[0] == '/') { // If we encounter "/index.html"
        p_start = uri;
    } else { // Cases without schema and without starting slash
        p_start = strchr(uri, '/'); // If we encounter "www.google.com/index.html"
        p_start = (p_start == NULL) ? uri : p_start;
    }

    // Copy chars between p_start and p_end to doc_path
    strncpy(doc_path, p_start, len);
    doc_path[p_end - p_start] = '\0';

    // Validate and fix doc_path with /index.html if ended up with invalid file path
    const char* sl = strrchr(doc_path, '/'); // Last slash
    const char* old_end = doc_path + strlen(doc_path);

    // When pre-validated doc_path is "www.google.com" (because uri was http://www.google.com or http://www.google.com or www.google.com)
    // This can be discerned if no forward-slash exists
    if (sl == NULL) { 
        strncat(doc_path, "/index.html", len - strlen(doc_path));
        // Validate that we managed to append this
        if (strcmp(old_end, "/index.html") != 0) { // We exceeded PATH_MAX length
            return 1;
        }
    } else if ((sl+1) == old_end) { // If there is nothing after last slash (e.g. www.google.com/, /, www.google.com/subdir/)
        strncat(doc_path, "index.html", len - strlen(doc_path));
        // Validate that we managed to append this
        if (strcmp(old_end, "index.html") != 0) { // We exceeded PATH_MAX length
            return 1;
        }
    }

    return 0;
}

// Parse raw received bytes into http request struct
// Returns 0 if parsing was successful, 1 if not then its a "400 Bad Request" because of malformed client message
// Example request below:
/*
GET /index.html HTTP/1.0\r\n          <--- <Method> <SP> <URI> <SP> <HTTP version> <CRLF>
Content-type: application/json\r\n    <--- start of entity-header fields
Host: www.google.com\r\n
Accept-Encoding: gzip, deflate\r\n    <--- end of entity-header fields
\r\n                                  <--- Termination signal \r\n\r\n (or also \n\n for tolerant approach)
*/
int parse_http_request(char* message_buf, http_request_t* http_request)
{
    // Extract raw tokens from Request-Line
    if (parse_request_line(message_buf, http_request) != 0) {
        return 1;
    }

    // Decode URI
    if (uri_decode(http_request->uri, http_request->uri) != 0) {
        return 1;
    }

    // Get doc_path from request uri
    parse_doc_path_uri(http_request->doc_path, http_request->uri, PATH_MAX);

    return 0;
}

// Send HTTP response based on http_request through socket_id socket
// Return 0 if sending was successful, 1 if nothing succeeded (in which case you want to close connection)
// Example response below:
/*
HTTP/1.0 200 OK\r\n                            <--- <HTTP version> <SP> <Status code> <SP> <Status name/description> <CRLF>
Date: Wed, 10 Oct 2018 18:39:41 GMT
Content-Type: text/html
Content-Length: 1087
Last-Modified: Wed, 10 Oct 2018 15:26:09 GMT
Server: BTH students
\r\n                                           <--- Header/Body divider
<html>                                         <--- Body content start
  <body>
    ...
  </body>
</html>                                        <--- We close connection when body is fully sent (body being contents of some document/resource)
*/
int send_http_response(int socket_id, const config_t* conf, const http_request_t* http_request)
{
    int request_get = 0; // 0 - HEAD, 1 - GET
    http_status_t status = HTTP_STATUS_OK;
    time_t time_now;
    char str_date[100];
    char str_last_modified[100];
    char response_msg[CONF_REQ_BUFSIZE];
    char socket_buf[CONF_SOCK_BUFSIZE];
    char resolved_path[PATH_MAX];
    struct stat doc_stats;
    struct tm tm_date;
    struct tm tm_last_modified;
    int read_bytes, write_bytes; // For write/read return values
    int fd; // For converting FILE* pointer to int fd
    FILE* doc_file;

    // Check if version is correct (allowing: 1.0, 1.1 and 2.0), if not - send 400 - Bad Request
    if (strcmp(HTTP_VERSION_1_0, http_request->version) != 0 && 
        strcmp(HTTP_VERSION_1_1, http_request->version) != 0 &&
        strcmp(HTTP_VERSION_2_0, http_request->version) != 0) {
        return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_BADREQUEST);
    }

    // Check if request type is implemented and determine whether its a get request
    if (strcmp(HTTP_METHOD_GET, http_request->method) == 0) {
        request_get = 1;
    } else if (strcmp(HTTP_METHOD_HEAD, http_request->method) != 0) {
        return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_NOTIMPLEMENTED);
    }
    
    // Get realpath of doc_path_full
    if (realpath(http_request->doc_path, resolved_path) == NULL) {
        if (errno == ENOENT) {
            return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_NOTFOUND);
        } else if (errno == EACCES) {
            return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_FORBIDDEN);
        } else {
            return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_BADREQUEST);
        }
    }

    // Simple Forbidden demonstration (we wont allow users to directly access _errors folder)
    if (strncmp("/_errors/", resolved_path, strlen("/_errors/")) == 0) {
        return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_FORBIDDEN);
    }

    // Try to stat the file
    if (stat(resolved_path, &doc_stats) != 0) {
        if (errno == ENOENT) { // File not found
            return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_NOTFOUND);
        } else if (errno == EACCES) { // We don't have permission to it (not even to stat it)
            return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_FORBIDDEN);
        } else if (errno == ENAMETOOLONG) { // Pathname too long, not sure if to return 400 or 403
            return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_BADREQUEST);
        } else { // If we get something else for whatever reason
            return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_INTERNALSERVERERROR);
        }
    }
    
    // Handle dates
    time_now = time(0);
    if (gmtime_r(&time_now, &tm_date) == NULL) { // Convert current time (Date header) to GMT timestamp
        return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_INTERNALSERVERERROR);
    }
    if (gmtime_r(&doc_stats.st_ctime, &tm_last_modified) == NULL) { // Convert Last-Modified to GMT timestamp
        return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_INTERNALSERVERERROR);
    }

    // Format dates
    strftime(str_date, 100, HTTP_DATETIME_FORMAT, &tm_date);
    strftime(str_last_modified, 100, HTTP_DATETIME_FORMAT, &tm_last_modified);

    // Prepare header of response message
    snprintf(response_msg, CONF_REQ_BUFSIZE, 
        "%s %d %s\r\n"
        "Date: %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Last-Modified: %s\r\n"
        "Server: %s\r\n"
        "\r\n",
        HTTP_VERSION, status, http_status_str(status),
        str_date,
        doc_content_type(http_request->doc_path),
        doc_stats.st_size,
        str_last_modified,
        HTTP_HEADER_SERVER);


    // If request is GET, then transmit Header AND doc_file contents
    // Else request is HEAD, therefore, transfer only Header
    if (request_get) {      
        if ((doc_file = fopen(resolved_path, "r")) == NULL) { // Since file errors should be cought by stat(...), this is unexpected, therefore 500 error
            return send_http_error_response(socket_id, conf, http_request, HTTP_STATUS_INTERNALSERVERERROR);
        }
        fd = fileno(doc_file);

        // Transmit response header part
        write_bytes = write(socket_id, response_msg, strlen(response_msg));
        if (write_bytes < 0) { // Something wrong with socket during header write
            return 1;
        }        

        // Transmit body/file part
        while((read_bytes = read(fd, socket_buf, CONF_SOCK_BUFSIZE)) > 0) {
            write_bytes = write(socket_id, socket_buf, read_bytes);

            if (write_bytes < 0) { // Something wrong with socket during file streaming
                return 1;
            }
        }        
    
        fclose(doc_file);

        if (read_bytes < 0) { // Something wrong with file during reading
            return 1;
        }
    } else {
        write_bytes = write(socket_id, response_msg, strlen(response_msg));
        if (write_bytes < 0) { // Something wrong with socket during header write
            return 1;
        }
    }

    printf("[INFO] [socket: %d] Client: \"%s %s %s\" => Server: \"%s %d %s\"\n", 
        socket_id, 
        http_request->method, http_request->uri, http_request->version,
        HTTP_VERSION, status, http_status_str(status));
    return 0;
}

// Send formatted status error response based on status code
int send_http_error_response(int socket_id, const config_t* conf, const http_request_t* http_request, http_status_t status)
{
    int use_hardcoded = 0; // 0 - Use .html file for error, 1 - Use hardcoded format
    time_t time_now;
    char str_date[100];
    char response_msg[CONF_REQ_BUFSIZE];
    char socket_buf[CONF_SOCK_BUFSIZE];
    char err_path[PATH_MAX];
    struct stat errf_stats;
    struct tm tm_date;
    long int content_length;
    int read_bytes, write_bytes; // For write/read return values
    int fd; // For converting FILE* pointer to int fd
    FILE* err_file;
    
    // Get correct error file path
    snprintf(err_path, PATH_MAX, "/_errors/%d.html", status);

    // Handle & format dates
    time_now = time(0);
    if (gmtime_r(&time_now, &tm_date) == NULL) { // Convert current time (Date header) to GMT timestamp
        return 1;
    }
    strftime(str_date, 100, HTTP_DATETIME_FORMAT, &tm_date);    

    // Try to stat the error file
    if (stat(err_path, &errf_stats) != 0) {
        printf("[WARN] [socket: %d] [send_http_error_response] Status file path \"%s\" error: %s, using hardcoded ...\n", socket_id, err_path, strerror(errno));

        use_hardcoded = 1;
        snprintf(socket_buf, CONF_SOCK_BUFSIZE, HTTP_STATUS_HTML_SIMPLE, status, http_status_str(status), status, http_status_str(status));
        content_length = strlen(socket_buf);
    } else {
        content_length = errf_stats.st_size;
    }

    // Prepare header of response message
    snprintf(response_msg, CONF_REQ_BUFSIZE, 
        "%s %d %s\r\n"
        "Date: %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Server: %s\r\n"
        "\r\n",
        HTTP_VERSION, status, http_status_str(status),
        str_date,
        CONTENT_TEXT_HTML,
        content_length,
        HTTP_HEADER_SERVER);

    // If we don't use hardcoded error status response, send header AND error status file contents
    // Else use formatted, hardcoded HTML string for error status content sending
    if (!use_hardcoded) {
        // Since file errors should be cought by stat(...), this is unexpected, therefore no other options than to close connection
        if ((err_file = fopen(err_path, "r")) == NULL) { 
            return 1;
        }
        fd = fileno(err_file);

        // Transmit response header part
        write_bytes = write(socket_id, response_msg, strlen(response_msg));
        if (write_bytes < 0) { // Something wrong with socket during header write
            return 1;
        }        

        // Transmit body/file part
        while((read_bytes = read(fd, socket_buf, CONF_SOCK_BUFSIZE)) > 0) {
            write_bytes = write(socket_id, socket_buf, read_bytes);

            if (write_bytes < 0) { // Something wrong with socket during file streaming
                return 1;
            }
        }        
    
        fclose(err_file);

        if (read_bytes < 0) { // Something wrong with file during reading
            return 1;
        }
    } else {
        // Transmit response header part
        write_bytes = write(socket_id, response_msg, strlen(response_msg));
        if (write_bytes < 0) { // Something wrong with socket during header write
            return 1;
        }

        // Transmit hardcoded HTML error content
        write_bytes = write(socket_id, socket_buf, content_length);
        if (write_bytes < 0) { // Something wrong with socket during header write
            return 1;
        }
    }

    if (http_request != NULL) {
        printf("[INFO] [socket: %d] Client: \"%s %s %s\" => Server: \"%s %d %s\"\n", 
            socket_id, 
            http_request->method, http_request->uri, http_request->version,
            HTTP_VERSION, status, http_status_str(status));
    } else {
        printf("[INFO] [socket: %d] Client: \" ... \" => Server: \"%s %d %s\"\n", 
            socket_id, 
            HTTP_VERSION, status, http_status_str(status));
    }

    return 0;
}