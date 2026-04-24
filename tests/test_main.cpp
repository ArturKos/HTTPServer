#include <gtest/gtest.h>

int main(int argument_count, char** argument_values)
{
    ::testing::InitGoogleTest(&argument_count, argument_values);
    return RUN_ALL_TESTS();
}
