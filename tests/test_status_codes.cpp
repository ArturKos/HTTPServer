#include <gtest/gtest.h>

#include "http_server/status_codes.h"

TEST(HttpStatusReasonPhrase, ReturnsKnownPhrases)
{
    EXPECT_STREQ(http_status_get_reason_phrase(200), "OK");
    EXPECT_STREQ(http_status_get_reason_phrase(400), "Bad Request");
    EXPECT_STREQ(http_status_get_reason_phrase(403), "Forbidden");
    EXPECT_STREQ(http_status_get_reason_phrase(404), "Not Found");
    EXPECT_STREQ(http_status_get_reason_phrase(405), "Method Not Allowed");
    EXPECT_STREQ(http_status_get_reason_phrase(500), "Internal Server Error");
    EXPECT_STREQ(http_status_get_reason_phrase(501), "Not Implemented");
}

TEST(HttpStatusReasonPhrase, FallsBackForUnknown)
{
    EXPECT_STREQ(http_status_get_reason_phrase(999), "Unknown");
    EXPECT_STREQ(http_status_get_reason_phrase(0),   "Unknown");
}
