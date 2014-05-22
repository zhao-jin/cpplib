// Copyright (c) 2011, Tencent Inc. All rights reserved.
// Author:  Wen Jie <welkinwen@tencent.com>
// Created: 2011-11-16

#include "common/web/url_util.h"
#include "thirdparty/gtest/gtest.h"

TEST(UrlUtil, GetParentOfReversedSegmentPath)
{
    std::string reversed_url = "com.sample.www\"http\"/a/b/c/";
    std::string parent = web::url::UrlUtil::GetParentOfReversedSegmentPath(reversed_url);

    EXPECT_EQ("com.sample.www\"http\"/a/b/", parent);
}

TEST(UrlUtil, GetPrefixesOfUrl)
{
    std::string url_string = "http://www.sample.com/a/b/c/d/e.html";
    std::vector<std::string> prefixes;
    web::url::UrlUtil::GetPrefixesOfUrl(url_string, prefixes);
    uint32_t prefixes_count = 5;
    ASSERT_EQ(prefixes_count, prefixes.size());
    EXPECT_EQ("http://www.sample.com/", prefixes[0]);
    EXPECT_EQ("http://www.sample.com/a/", prefixes[1]);
    EXPECT_EQ("http://www.sample.com/a/b/", prefixes[2]);
    EXPECT_EQ("http://www.sample.com/a/b/c/", prefixes[3]);
    EXPECT_EQ("http://www.sample.com/a/b/c/d/", prefixes[4]);

    prefixes.clear();
    web::url::Url url;
    ASSERT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    web::url::UrlUtil::GetPrefixesOfUrl(url, prefixes);
    ASSERT_EQ(prefixes_count, prefixes.size());
    EXPECT_EQ("http://www.sample.com/", prefixes[0]);
    EXPECT_EQ("http://www.sample.com/a/", prefixes[1]);
    EXPECT_EQ("http://www.sample.com/a/b/", prefixes[2]);
    EXPECT_EQ("http://www.sample.com/a/b/c/", prefixes[3]);
    EXPECT_EQ("http://www.sample.com/a/b/c/d/", prefixes[4]);
}

TEST(UrlUtil, GetReversedPrefixesOfUrl)
{
    std::string url_string = "http://www.sample.com/a/b/c/d/e.html";
    std::vector<std::string> reversed_prefixes;
    web::url::UrlUtil::GetReversedPrefixesOfUrl(url_string, reversed_prefixes);
    uint32_t prefixes_count = 5;
    ASSERT_EQ(prefixes_count, reversed_prefixes.size());
    EXPECT_EQ("com.sample.www\"http\"/", reversed_prefixes[0]);
    EXPECT_EQ("com.sample.www\"http\"/a/", reversed_prefixes[1]);
    EXPECT_EQ("com.sample.www\"http\"/a/b/", reversed_prefixes[2]);
    EXPECT_EQ("com.sample.www\"http\"/a/b/c/", reversed_prefixes[3]);
    EXPECT_EQ("com.sample.www\"http\"/a/b/c/d/", reversed_prefixes[4]);

    reversed_prefixes.clear();
    web::url::Url url;
    ASSERT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    web::url::UrlUtil::GetReversedPrefixesOfUrl(url, reversed_prefixes);
    ASSERT_EQ(prefixes_count, reversed_prefixes.size());
    EXPECT_EQ("com.sample.www\"http\"/", reversed_prefixes[0]);
    EXPECT_EQ("com.sample.www\"http\"/a/", reversed_prefixes[1]);
    EXPECT_EQ("com.sample.www\"http\"/a/b/", reversed_prefixes[2]);
    EXPECT_EQ("com.sample.www\"http\"/a/b/c/", reversed_prefixes[3]);
    EXPECT_EQ("com.sample.www\"http\"/a/b/c/d/", reversed_prefixes[4]);
}

TEST(UrlUtil, GetPrefixesOfReversedUrl)
{
    std::string reversed_url_string = "com.sample.www\"http\"/a/b/c/d/e.html";
    std::vector<std::string> prefixes;
    web::url::UrlUtil::GetPrefixesOfReversedUrl(reversed_url_string, prefixes);
    uint32_t prefixes_count = 5;
    ASSERT_EQ(prefixes_count, prefixes.size());
    EXPECT_EQ("http://www.sample.com/", prefixes[0]);
    EXPECT_EQ("http://www.sample.com/a/", prefixes[1]);
    EXPECT_EQ("http://www.sample.com/a/b/", prefixes[2]);
    EXPECT_EQ("http://www.sample.com/a/b/c/", prefixes[3]);
    EXPECT_EQ("http://www.sample.com/a/b/c/d/", prefixes[4]);
}

TEST(UrlUtil, GetReversedPrefixesOfReversedUrl)
{
    std::string reversed_url_string = "com.sample.www\"http\"/a/b/c/d/e.html";
    std::vector<std::string> reversed_prefixes;
    web::url::UrlUtil::GetReversedPrefixesOfReversedUrl(reversed_url_string, reversed_prefixes);
    uint32_t prefixes_count = 5;
    ASSERT_EQ(prefixes_count, reversed_prefixes.size());
    EXPECT_EQ("com.sample.www\"http\"/", reversed_prefixes[0]);
    EXPECT_EQ("com.sample.www\"http\"/a/", reversed_prefixes[1]);
    EXPECT_EQ("com.sample.www\"http\"/a/b/", reversed_prefixes[2]);
    EXPECT_EQ("com.sample.www\"http\"/a/b/c/", reversed_prefixes[3]);
    EXPECT_EQ("com.sample.www\"http\"/a/b/c/d/", reversed_prefixes[4]);
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
