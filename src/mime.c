#include "http_server/mime.h"

#include <ctype.h>
#include <string.h>

typedef struct MimeEntry {
    const char* file_extension;
    const char* content_type;
} MimeEntry;

static const MimeEntry kMimeTable[] = {
    {"html", "text/html; charset=utf-8"},
    {"htm",  "text/html; charset=utf-8"},
    {"css",  "text/css; charset=utf-8"},
    {"js",   "application/javascript; charset=utf-8"},
    {"json", "application/json; charset=utf-8"},
    {"txt",  "text/plain; charset=utf-8"},
    {"xml",  "application/xml; charset=utf-8"},
    {"png",  "image/png"},
    {"jpg",  "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"gif",  "image/gif"},
    {"svg",  "image/svg+xml"},
    {"ico",  "image/x-icon"},
    {"pdf",  "application/pdf"},
    {"zip",  "application/zip"},
    {"wasm", "application/wasm"},
};

static const char kDefaultContentType[] = "application/octet-stream";

static int case_insensitive_equal(const char* left, const char* right)
{
    while (*left && *right) {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) {
            return 0;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

const char* mime_get_content_type(const char* file_extension)
{
    if (file_extension == NULL || file_extension[0] == '\0') {
        return kDefaultContentType;
    }

    const size_t entry_count = sizeof(kMimeTable) / sizeof(kMimeTable[0]);
    for (size_t entry_index = 0; entry_index < entry_count; ++entry_index) {
        if (case_insensitive_equal(file_extension, kMimeTable[entry_index].file_extension)) {
            return kMimeTable[entry_index].content_type;
        }
    }
    return kDefaultContentType;
}
