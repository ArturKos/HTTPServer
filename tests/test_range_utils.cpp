#include <gtest/gtest.h>

#include "http_server/range_utils.h"

TEST(RangeHeaderParse, ReportsAbsentWhenNullOrEmpty)
{
    ByteRange byte_range{};
    EXPECT_EQ(range_header_parse(nullptr, 1000, &byte_range), RANGE_PARSE_ABSENT);
    EXPECT_EQ(range_header_parse("", 1000, &byte_range), RANGE_PARSE_ABSENT);
}

TEST(RangeHeaderParse, ClosedRange)
{
    ByteRange byte_range{};
    ASSERT_EQ(range_header_parse("bytes=0-499", 1000, &byte_range), RANGE_PARSE_OK);
    EXPECT_EQ(byte_range.first_byte_offset, 0u);
    EXPECT_EQ(byte_range.last_byte_offset, 499u);
}

TEST(RangeHeaderParse, OpenEndedRange)
{
    ByteRange byte_range{};
    ASSERT_EQ(range_header_parse("bytes=500-", 1000, &byte_range), RANGE_PARSE_OK);
    EXPECT_EQ(byte_range.first_byte_offset, 500u);
    EXPECT_EQ(byte_range.last_byte_offset, 999u);
}

TEST(RangeHeaderParse, SuffixRange)
{
    ByteRange byte_range{};
    ASSERT_EQ(range_header_parse("bytes=-200", 1000, &byte_range), RANGE_PARSE_OK);
    EXPECT_EQ(byte_range.first_byte_offset, 800u);
    EXPECT_EQ(byte_range.last_byte_offset, 999u);
}

TEST(RangeHeaderParse, SuffixLongerThanResourceClampsToBeginning)
{
    ByteRange byte_range{};
    ASSERT_EQ(range_header_parse("bytes=-5000", 1000, &byte_range), RANGE_PARSE_OK);
    EXPECT_EQ(byte_range.first_byte_offset, 0u);
    EXPECT_EQ(byte_range.last_byte_offset, 999u);
}

TEST(RangeHeaderParse, EndBeyondResourceIsClamped)
{
    ByteRange byte_range{};
    ASSERT_EQ(range_header_parse("bytes=100-9999", 1000, &byte_range), RANGE_PARSE_OK);
    EXPECT_EQ(byte_range.first_byte_offset, 100u);
    EXPECT_EQ(byte_range.last_byte_offset, 999u);
}

TEST(RangeHeaderParse, FirstOffsetBeyondResourceIsNotSatisfiable)
{
    ByteRange byte_range{};
    EXPECT_EQ(range_header_parse("bytes=2000-3000", 1000, &byte_range),
              RANGE_PARSE_NOT_SATISFIABLE);
}

TEST(RangeHeaderParse, RejectsMultiRange)
{
    ByteRange byte_range{};
    EXPECT_EQ(range_header_parse("bytes=0-10,20-30", 1000, &byte_range),
              RANGE_PARSE_UNSUPPORTED);
}

TEST(RangeHeaderParse, RejectsNonBytesUnit)
{
    ByteRange byte_range{};
    EXPECT_EQ(range_header_parse("items=0-1", 1000, &byte_range),
              RANGE_PARSE_UNSUPPORTED);
}

TEST(RangeHeaderParse, RejectsInvertedRange)
{
    ByteRange byte_range{};
    EXPECT_EQ(range_header_parse("bytes=500-100", 1000, &byte_range),
              RANGE_PARSE_UNSUPPORTED);
}
