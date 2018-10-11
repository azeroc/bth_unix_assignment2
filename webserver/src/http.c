#include <http.h>

const char * httpStatusStr(http_status_t status)
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