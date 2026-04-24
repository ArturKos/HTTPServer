#include <gtest/gtest.h>

#include "http_server/mime.h"

#include <cstring>

TEST(MimeContentType, ReturnsHtmlForHtmlExtension)
{
    EXPECT_STREQ(mime_get_content_type("html"), "text/html; charset=utf-8");
    EXPECT_STREQ(mime_get_content_type("htm"),  "text/html; charset=utf-8");
}

TEST(MimeContentType, IsCaseInsensitive)
{
    EXPECT_STREQ(mime_get_content_type("HTML"), "text/html; charset=utf-8");
    EXPECT_STREQ(mime_get_content_type("Png"),  "image/png");
    EXPECT_STREQ(mime_get_content_type("JpG"),  "image/jpeg");
}

TEST(MimeContentType, RecognisesCommonTypes)
{
    EXPECT_STREQ(mime_get_content_type("css"),  "text/css; charset=utf-8");
    EXPECT_STREQ(mime_get_content_type("js"),   "application/javascript; charset=utf-8");
    EXPECT_STREQ(mime_get_content_type("json"), "application/json; charset=utf-8");
    EXPECT_STREQ(mime_get_content_type("png"),  "image/png");
    EXPECT_STREQ(mime_get_content_type("jpeg"), "image/jpeg");
    EXPECT_STREQ(mime_get_content_type("gif"),  "image/gif");
    EXPECT_STREQ(mime_get_content_type("svg"),  "image/svg+xml");
    EXPECT_STREQ(mime_get_content_type("pdf"),  "application/pdf");
}

TEST(MimeContentType, FallsBackToOctetStreamForUnknown)
{
    EXPECT_STREQ(mime_get_content_type("xyz"), "application/octet-stream");
    EXPECT_STREQ(mime_get_content_type(""),    "application/octet-stream");
    EXPECT_STREQ(mime_get_content_type(nullptr), "application/octet-stream");
}
