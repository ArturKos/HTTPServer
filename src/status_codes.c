#include "http_server/status_codes.h"

const char* http_status_get_reason_phrase(int status_code)
{
    switch (status_code) {
        case HTTP_STATUS_OK:                    return "OK";
        case HTTP_STATUS_PARTIAL_CONTENT:       return "Partial Content";
        case HTTP_STATUS_BAD_REQUEST:           return "Bad Request";
        case HTTP_STATUS_FORBIDDEN:             return "Forbidden";
        case HTTP_STATUS_NOT_FOUND:             return "Not Found";
        case HTTP_STATUS_METHOD_NOT_ALLOWED:    return "Method Not Allowed";
        case HTTP_STATUS_RANGE_NOT_SATISFIABLE: return "Range Not Satisfiable";
        case HTTP_STATUS_INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HTTP_STATUS_NOT_IMPLEMENTED:       return "Not Implemented";
        default:                                return "Unknown";
    }
}
