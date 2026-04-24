#include <gtest/gtest.h>

#include "http_server/request.h"

#include <cstring>

TEST(HttpRequestParse, ParsesSimpleGetRequest)
{
    const char raw_request[] = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest parsed_request;
    ASSERT_TRUE(http_request_parse(raw_request, sizeof(raw_request) - 1, &parsed_request));
    EXPECT_EQ(parsed_request.method, HTTP_METHOD_GET);
    EXPECT_STREQ(parsed_request.path, "/index.html");
    EXPECT_STREQ(parsed_request.version, "HTTP/1.1");
    EXPECT_STREQ(parsed_request.query_string, "");
}

TEST(HttpRequestParse, ParsesQueryString)
{
    const char raw_request[] = "GET /search?q=hello&lang=en HTTP/1.1\r\n\r\n";
    HttpRequest parsed_request;
    ASSERT_TRUE(http_request_parse(raw_request, sizeof(raw_request) - 1, &parsed_request));
    EXPECT_STREQ(parsed_request.path, "/search");
    EXPECT_STREQ(parsed_request.query_string, "q=hello&lang=en");
}

TEST(HttpRequestParse, DecodesUrlEncodedPath)
{
    const char raw_request[] = "GET /a%20b%2Fc HTTP/1.1\r\n\r\n";
    HttpRequest parsed_request;
    ASSERT_TRUE(http_request_parse(raw_request, sizeof(raw_request) - 1, &parsed_request));
    EXPECT_STREQ(parsed_request.path, "/a b/c");
}

TEST(HttpRequestParse, ParsesPostWithContentLength)
{
    const char raw_request[] =
        "POST /submit HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "name=john_doe";
    HttpRequest parsed_request;
    ASSERT_TRUE(http_request_parse(raw_request, sizeof(raw_request) - 1, &parsed_request));
    EXPECT_EQ(parsed_request.method, HTTP_METHOD_POST);
    EXPECT_STREQ(parsed_request.path, "/submit");
    EXPECT_EQ(parsed_request.content_length, 13u);
    EXPECT_STREQ(parsed_request.content_type, "application/x-www-form-urlencoded");
}

TEST(HttpRequestParse, ParsesHeadRequest)
{
    const char raw_request[] = "HEAD /resource HTTP/1.1\r\n\r\n";
    HttpRequest parsed_request;
    ASSERT_TRUE(http_request_parse(raw_request, sizeof(raw_request) - 1, &parsed_request));
    EXPECT_EQ(parsed_request.method, HTTP_METHOD_HEAD);
}

TEST(HttpRequestParse, RejectsUnsupportedMethod)
{
    const char raw_request[] = "DELETE /resource HTTP/1.1\r\n\r\n";
    HttpRequest parsed_request;
    EXPECT_FALSE(http_request_parse(raw_request, sizeof(raw_request) - 1, &parsed_request));
}

TEST(HttpRequestParse, RejectsMalformedRequestLine)
{
    const char raw_request[] = "GET\r\n\r\n";
    HttpRequest parsed_request;
    EXPECT_FALSE(http_request_parse(raw_request, sizeof(raw_request) - 1, &parsed_request));
}

TEST(HttpRequestKeepAlive, Http11DefaultsToKeepAlive)
{
    const char raw_request[] = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
    HttpRequest parsed_request;
    ASSERT_TRUE(http_request_parse(raw_request, sizeof(raw_request) - 1, &parsed_request));
    EXPECT_TRUE(http_request_is_keep_alive(&parsed_request));
}

TEST(HttpRequestKeepAlive, Http11ClosesOnExplicitCloseHeader)
{
    const char raw_request[] = "GET / HTTP/1.1\r\nConnection: close\r\n\r\n";
    HttpRequest parsed_request;
    ASSERT_TRUE(http_request_parse(raw_request, sizeof(raw_request) - 1, &parsed_request));
    EXPECT_FALSE(http_request_is_keep_alive(&parsed_request));
}

TEST(HttpRequestKeepAlive, Http10ClosesByDefault)
{
    const char raw_request[] = "GET / HTTP/1.0\r\nHost: x\r\n\r\n";
    HttpRequest parsed_request;
    ASSERT_TRUE(http_request_parse(raw_request, sizeof(raw_request) - 1, &parsed_request));
    EXPECT_FALSE(http_request_is_keep_alive(&parsed_request));
}

TEST(HttpRequestKeepAlive, Http10OptsInViaHeader)
{
    const char raw_request[] = "GET / HTTP/1.0\r\nConnection: keep-alive\r\n\r\n";
    HttpRequest parsed_request;
    ASSERT_TRUE(http_request_parse(raw_request, sizeof(raw_request) - 1, &parsed_request));
    EXPECT_TRUE(http_request_is_keep_alive(&parsed_request));
}

TEST(HttpMethodToString, ReturnsCanonicalStrings)
{
    EXPECT_STREQ(http_method_to_string(HTTP_METHOD_GET),     "GET");
    EXPECT_STREQ(http_method_to_string(HTTP_METHOD_HEAD),    "HEAD");
    EXPECT_STREQ(http_method_to_string(HTTP_METHOD_POST),    "POST");
    EXPECT_STREQ(http_method_to_string(HTTP_METHOD_UNKNOWN), "UNKNOWN");
}
