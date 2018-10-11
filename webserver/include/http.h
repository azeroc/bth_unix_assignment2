#ifndef HTTP_H
#define HTTP_H
#include <common.h>
#include <config.h>

// Constants
#define HTTP_MAX_URI_LENGTH 8000 // RFC 7230 recommendation, ref: https://tools.ietf.org/html/rfc7230#section-3.1.1

// HTTP version
#define HTTP_VERSION "HTTP/1.0"

// Valid methods
#define HTTP_METHOD_HEAD "HEAD"
#define HTTP_METHOD_GET  "GET"

// Implemented status codes
typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_BADREQUEST = 400,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOTFOUND = 404,
    HTTP_STATUS_INTERNALSERVERERROR = 500,
    HTTP_STATUS_NOTIMPLEMENTED = 501,
} http_status_t;

const char * httpStatusStr(http_status_t status);

typedef struct {
    char method[10]; // Request-Line method
    char version[10]; // HTTP Version
    char req_uri[HTTP_MAX_URI_LENGTH]; // Request-URI
    char** header_fields; // 2D array, line-separated header values
    char* body; // Requested file's content (only for Responses)
} http_request_t;

#endif // HTTP_H