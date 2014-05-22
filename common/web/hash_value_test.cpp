// Copyright (c) 2011, Tencent Inc. All rights reserved.
// Author:  Wen Jie <welkinwen@tencent.com>
// Created: 2011-12-22

#include <inttypes.h>
#include "common/web/hash_value.h"
#include "thirdparty/gtest/gtest.h"

TEST(HashValue, SetValue)
{
    web::HashValue<96/8> hash_value;

    const unsigned char bytes_value_1[12] =
    { 0x0, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef };
    hash_value.SetValue(bytes_value_1);
    for (size_t i = 0; i < sizeof(bytes_value_1); ++i)
    {
        EXPECT_EQ(bytes_value_1[i], hash_value.AsBytes()[i]);
    }
    std::string bytes = hash_value.ToString();
    EXPECT_EQ(sizeof(bytes_value_1), bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        EXPECT_EQ(static_cast<uint8_t>(bytes_value_1[i]), static_cast<uint8_t>(bytes[i]));
    }
    EXPECT_EQ("1234567890abcdefabcdef", hash_value.ToHexString());
    EXPECT_EQ("001234567890abcdefabcdef", hash_value.ToHexString(false));

    const unsigned char bytes_value_2[12] =
    { 0x0, 0x01, 0x23, 0x45, 0x67, 0x89, 0x0a, 0xbc, 0xde, 0xfa, 0xbc, 0xde };
    hash_value.SetValue(bytes_value_2);
    for (size_t i = 0; i < sizeof(bytes_value_2); ++i)
    {
        EXPECT_EQ(bytes_value_2[i], hash_value.AsBytes()[i]);
    }
    bytes = hash_value.ToString();
    EXPECT_EQ(sizeof(bytes_value_2), bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        EXPECT_EQ(static_cast<uint8_t>(bytes_value_2[i]), static_cast<uint8_t>(bytes[i]));
    }
    EXPECT_EQ("1234567890abcdefabcde", hash_value.ToHexString());
    EXPECT_EQ("0001234567890abcdefabcde", hash_value.ToHexString(false));
}

TEST(HashValue, ParseFromHexString)
{
    web::HashValue<96/8> hash_value;

    const unsigned char bytes_value_1[12] =
    { 0x0, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef };
    EXPECT_TRUE(hash_value.ParseFromHexString("1234567890ABCDEFabcdef"));
    for (size_t i = 0; i < sizeof(bytes_value_1); ++i)
    {
        EXPECT_EQ(bytes_value_1[i], hash_value.AsBytes()[i]);
    }
    std::string bytes = hash_value.ToString();
    EXPECT_EQ(sizeof(bytes_value_1), bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        EXPECT_EQ(static_cast<uint8_t>(bytes_value_1[i]), static_cast<uint8_t>(bytes[i]));
    }
    EXPECT_EQ("1234567890abcdefabcdef", hash_value.ToHexString());
    EXPECT_EQ("001234567890abcdefabcdef", hash_value.ToHexString(false));

    const unsigned char bytes_value_2[12] =
    { 0x0, 0x01, 0x23, 0x45, 0x67, 0x89, 0x0a, 0xbc, 0xde, 0xfa, 0xbc, 0xde };
    EXPECT_TRUE(hash_value.ParseFromHexString("1234567890ABCDEFabcde"));
    for (size_t i = 0; i < sizeof(bytes_value_2); ++i)
    {
        EXPECT_EQ(bytes_value_2[i], hash_value.AsBytes()[i]);
    }
    bytes = hash_value.ToString();
    EXPECT_EQ(sizeof(bytes_value_2), bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        EXPECT_EQ(static_cast<uint8_t>(bytes_value_2[i]), static_cast<uint8_t>(bytes[i]));
    }
    EXPECT_EQ("1234567890abcdefabcde", hash_value.ToHexString());
    EXPECT_EQ("0001234567890abcdefabcde", hash_value.ToHexString(false));

    EXPECT_TRUE(hash_value.ParseFromHexString("0"));
    EXPECT_TRUE(hash_value.IsZero());
    EXPECT_EQ("0", hash_value.ToHexString());

    EXPECT_TRUE(hash_value.ParseFromHexString("000000000abcdef"));
    EXPECT_FALSE(hash_value.IsZero());
    EXPECT_EQ("abcdef", hash_value.ToHexString());

    EXPECT_FALSE(hash_value.ParseFromHexString("abcdefghijk"));
    EXPECT_FALSE(hash_value.ParseFromHexString("1234567890ABCDEFabcdefabcdeef"));
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
