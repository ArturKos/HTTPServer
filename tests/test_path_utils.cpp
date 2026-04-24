#include <gtest/gtest.h>

#include "http_server/path_utils.h"

#include <cstring>

TEST(UrlDecode, PreservesPlainAscii)
{
    char decoded_buffer[64];
    ASSERT_TRUE(url_decode("/index.html", decoded_buffer, sizeof(decoded_buffer)));
    EXPECT_STREQ(decoded_buffer, "/index.html");
}

TEST(UrlDecode, TranslatesPercentEncoding)
{
    char decoded_buffer[64];
    ASSERT_TRUE(url_decode("/hello%20world", decoded_buffer, sizeof(decoded_buffer)));
    EXPECT_STREQ(decoded_buffer, "/hello world");

    ASSERT_TRUE(url_decode("/a%2Fb", decoded_buffer, sizeof(decoded_buffer)));
    EXPECT_STREQ(decoded_buffer, "/a/b");
}

TEST(UrlDecode, TranslatesPlusToSpace)
{
    char decoded_buffer[64];
    ASSERT_TRUE(url_decode("/search?q=a+b", decoded_buffer, sizeof(decoded_buffer)));
    EXPECT_STREQ(decoded_buffer, "/search?q=a b");
}

TEST(UrlDecode, RejectsMalformedPercentEscape)
{
    char decoded_buffer[64];
    EXPECT_FALSE(url_decode("/bad%XY", decoded_buffer, sizeof(decoded_buffer)));
    EXPECT_FALSE(url_decode("/bad%2",  decoded_buffer, sizeof(decoded_buffer)));
}

TEST(UrlDecode, RejectsTooSmallBuffer)
{
    char decoded_buffer[4];
    EXPECT_FALSE(url_decode("/abcdef", decoded_buffer, sizeof(decoded_buffer)));
}

TEST(IsRequestPathSafe, AcceptsNormalPaths)
{
    EXPECT_TRUE(is_request_path_safe("/"));
    EXPECT_TRUE(is_request_path_safe("/index.html"));
    EXPECT_TRUE(is_request_path_safe("/assets/style.css"));
    EXPECT_TRUE(is_request_path_safe("/deep/nested/path/file.png"));
}

TEST(IsRequestPathSafe, RejectsParentReferences)
{
    EXPECT_FALSE(is_request_path_safe("/../etc/passwd"));
    EXPECT_FALSE(is_request_path_safe("/a/../b"));
    EXPECT_FALSE(is_request_path_safe("/a/.."));
    EXPECT_FALSE(is_request_path_safe("/.."));
}

TEST(IsRequestPathSafe, AcceptsDotsInsideNames)
{
    EXPECT_TRUE(is_request_path_safe("/a..b"));
    EXPECT_TRUE(is_request_path_safe("/file.tar.gz"));
}

TEST(IsRequestPathSafe, RejectsRelativeAndNull)
{
    EXPECT_FALSE(is_request_path_safe(nullptr));
    EXPECT_FALSE(is_request_path_safe("no-leading-slash"));
}

TEST(SplitPathAndQuery, SplitsCorrectly)
{
    char path_buffer[64];
    char query_buffer[64];
    ASSERT_TRUE(split_path_and_query("/search?q=hello",
                                     path_buffer, sizeof(path_buffer),
                                     query_buffer, sizeof(query_buffer)));
    EXPECT_STREQ(path_buffer,  "/search");
    EXPECT_STREQ(query_buffer, "q=hello");
}

TEST(SplitPathAndQuery, WithoutQueryClearsBuffer)
{
    char path_buffer[64];
    char query_buffer[64];
    ASSERT_TRUE(split_path_and_query("/index.html",
                                     path_buffer, sizeof(path_buffer),
                                     query_buffer, sizeof(query_buffer)));
    EXPECT_STREQ(path_buffer,  "/index.html");
    EXPECT_STREQ(query_buffer, "");
}

TEST(ExtractFileExtension, ReturnsLowercaseExtension)
{
    char extension_buffer[16];
    ASSERT_TRUE(extract_file_extension("/www/index.HTML",
                                       extension_buffer, sizeof(extension_buffer)));
    EXPECT_STREQ(extension_buffer, "html");
}

TEST(ExtractFileExtension, IgnoresDotsInDirectoryNames)
{
    char extension_buffer[16];
    ASSERT_TRUE(extract_file_extension("/a.b/c",
                                       extension_buffer, sizeof(extension_buffer)));
    EXPECT_STREQ(extension_buffer, "");
}

TEST(ExtractFileExtension, ReturnsEmptyWhenNoExtension)
{
    char extension_buffer[16];
    ASSERT_TRUE(extract_file_extension("/Makefile",
                                       extension_buffer, sizeof(extension_buffer)));
    EXPECT_STREQ(extension_buffer, "");
}

TEST(ResolveDocumentPath, AppendsIndexHtmlForTrailingSlash)
{
    char resolved_buffer[256];
    ASSERT_TRUE(resolve_document_path("./www", "/",
                                      resolved_buffer, sizeof(resolved_buffer)));
    EXPECT_STREQ(resolved_buffer, "./www/index.html");
}

TEST(ResolveDocumentPath, JoinsRootAndPath)
{
    char resolved_buffer[256];
    ASSERT_TRUE(resolve_document_path("/var/www", "/assets/a.css",
                                      resolved_buffer, sizeof(resolved_buffer)));
    EXPECT_STREQ(resolved_buffer, "/var/www/assets/a.css");
}

TEST(ResolveDocumentPath, HandlesTrailingSlashInRoot)
{
    char resolved_buffer[256];
    ASSERT_TRUE(resolve_document_path("/var/www/", "/index.html",
                                      resolved_buffer, sizeof(resolved_buffer)));
    EXPECT_STREQ(resolved_buffer, "/var/www/index.html");
}
