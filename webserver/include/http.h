#ifndef HTTP_H
#define HTTP_H
#include <common.h>
#include <config.h>

// Content-type defines/enums
#define CONTENT_TEXT_PLAIN       "text/plain"               // .txt
#define CONTENT_TEXT_HTML        "text/html"                // .htm .html
#define CONTENT_TEXT_CSS         "text/css"                 // .css
#define CONTENT_IMG_ICO          "image/x-icon"             // .ico
#define CONTENT_IMG_JPEG         "image/jpeg"               // .jpeg .jpg
#define CONTENT_IMG_PNG          "image/png"                // .png
#define CONTENT_IMG_GIF          "image/gif"                // .gif
#define CONTENT_APP_JS           "application/javascript"   // .js
#define CONTENT_APP_XML          "application/xml"          // .xml
#define CONTENT_APP_OCTET_STREAM "application/octet-stream" // .bin (any kind of binary data)
#define CONTENT_DEFAULT          CONTENT_APP_OCTET_STREAM   // .<any other> (default to binary data application/octet-stream)

// Get Content-Type from document/resource file extension
const char* doc_content_type(const char* docpath);

// Header configuration constants
#define HTTP_VERSION_1_0 "HTTP/1.0"
#define HTTP_VERSION_1_1 "HTTP/1.1"
#define HTTP_VERSION_2_0 "HTTP/2.0"
#define HTTP_VERSION HTTP_VERSION_1_0
#define HTTP_HEADER_SERVER "BTH students"
#define HTTP_DEFAULT_DATE "Thu, 1 Jan 1970 00:00:00 GMT" // Use in case of datetime formatting errors

// Formats
#define HTTP_DATETIME_FORMAT "%a, %d %b %Y %X GMT"

// Valid methods
#define HTTP_METHOD_HEAD "HEAD"
#define HTTP_METHOD_GET  "GET"

// Simple, hard-coded HTML status response format (when error files are not present)
// (Only use internally with defined statuses, do not allow client input)
#define HTTP_STATUS_HTML_SIMPLE "<!DOCTYPE html><html lang=en><head><title>%d %s</title></head><body><h1>HTTP 1.0 %d %s</h1></body></html>"

// Implemented status codes
typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_BADREQUEST = 400,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOTFOUND = 404,
    HTTP_STATUS_INTERNALSERVERERROR = 500,
    HTTP_STATUS_NOTIMPLEMENTED = 501,
} http_status_t;

const char * http_status_str(http_status_t status);

// Structure for HTTP request (GET,HEAD)
// Example request below:
/*
GET /index.html HTTP/1.0\r\n          <--- <Method> <SP> <URI> <SP> <HTTP version> <CRLF>
Content-type: application/json\r\n    <--- start of entity-header fields
Host: www.google.com\r\n
Accept-Encoding: gzip, deflate\r\n    <--- end of entity-header fields
\r\n                                  <--- Termination signal \r\n\r\n (or also \n\n for tolerant approach)
*/
typedef struct {
    char* message_buf; // Pointer to the raw message buffer
    char* method; // Pointer to the method
    char* uri; // Pointer to the URI
    char doc_path[PATH_MAX]; // Document path extracted and copied from URI
    char* version; // Pointer to the HTTP Version
    char* header_fields; // Pointer to Header fields
} http_request_t;

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

// Parse raw received bytes into http request struct
// Returns 0 if parsing was successful, 1 if not then its a "400 Bad Request" because of malformed client message
int parse_http_request(char* message_buf, http_request_t* http_request);

// Send HTTP response based on http_request through socket_id socket
// Return 0 if sending was successful, 1 if nothing succeeded (in which case you want to close connection)
int send_http_response(int socket_id, const config_t* conf, const http_request_t* http_request);

// Send formatted status error response based on status code
int send_http_error_response(int socket_id, const config_t* conf, const http_request_t* http_request, http_status_t status);

#endif // HTTP_H