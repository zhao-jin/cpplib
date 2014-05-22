// Copyright (c) 2011, Tencent Inc. All rights reserved.
// Author:  Wen Jie <welkinwen@tencent.com>
// Created: 2011-04-20

#include "common/web/url.h"
#include "thirdparty/gtest/gtest.h"

TEST(Url, GetAttribute)
{
    web::url::Url url;
    std::string url_string;

    url_string = "http://www.QQ.com:65535/x/Y/z/query.php?c=3&C=33&b=&b=2&D&a=1&";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_TRUE(url.IsValid());
    EXPECT_TRUE(url.IsDynamicPage());
    EXPECT_FALSE(url.IsHomePage());
    EXPECT_EQ("http://www.qq.com:65535/x/y/z/query.php?a=1&b=2&c=3&d", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www#65535\"http\"/x/y/z/query.php?a=1&b=2&c=3&d", url.GetReversedUrl());
    EXPECT_EQ("http://www.qq.com:65535/x/Y/z/query.php?a=1&b=2&C=33&c=3&D", url.GetAbsolutePath());
    EXPECT_STREQ("http://www.qq.com:65535/x/Y/z/query.php?a=1&b=2&C=33&c=3&D",
                 url.GetAbsolutePathPtr());
    EXPECT_EQ("/x/Y/z/query.php?a=1&b=2&C=33&c=3&D", url.GetRelativePath());
    EXPECT_STREQ("/x/Y/z/query.php?a=1&b=2&C=33&c=3&D", url.GetRelativePathPtr());
    EXPECT_EQ("http://www.qq.com:65535/x/Y/z/query.php", url.GetPortalPath());
    EXPECT_EQ("http://www.qq.com:65535/x/Y/z/query.php?", url.GetSegmentPath());
    EXPECT_EQ("com.qq.www#65535\"http\"/x/Y/z/query.php?", url.GetReversedSegmentPath());
    EXPECT_EQ("http://www.qq.com:65535/", url.GetHomePageURL());
    EXPECT_EQ(web::url::E_HTTP_PROTOCOL, url.GetScheme());
    EXPECT_EQ("http", url.GetSchemeString());
    EXPECT_EQ("www.qq.com", url.GetHost());
    EXPECT_STREQ("www.qq.com", url.GetHostPtr());
    EXPECT_EQ("www.qq.com:65535", url.GetHostPort());
    EXPECT_EQ("com.qq.www", url.GetReversedHost());
    EXPECT_EQ("com.qq.www#65535", url.GetReversedHostPort());
    EXPECT_EQ(65535, url.GetPort());
    EXPECT_EQ("/x/Y/z/query.php", url.GetPath());
    EXPECT_STREQ("/x/Y/z/query.php", url.GetPathPtr());
    std::map<std::string, std::string> querys;
    url.GetQuerys(querys);
    uint32_t querys_count = 5;
    ASSERT_EQ(querys_count, querys.size());
    EXPECT_EQ("1", querys["a"]);
    EXPECT_EQ("2", querys["b"]);
    EXPECT_EQ("3", querys["c"]);
    EXPECT_EQ("33", querys["C"]);
    EXPECT_EQ("", querys["D"]);
    EXPECT_EQ("/x/Y/z/", url.GetDirectory());
    std::vector<std::string> directories;
    url.GetDirectories(directories);
    uint32_t directories_count = 4;
    ASSERT_EQ(directories_count, directories.size());
    EXPECT_EQ("/", directories[0]);
    EXPECT_EQ("/x/", directories[1]);
    EXPECT_EQ("/x/Y/", directories[2]);
    EXPECT_EQ("/x/Y/z/", directories[3]);
    EXPECT_EQ("query.php", url.GetResource());
    EXPECT_STREQ("query.php", url.GetResourcePtr());
    EXPECT_EQ("php", url.GetResourceSuffix());
    EXPECT_STREQ("php", url.GetResourceSuffixPtr());
    EXPECT_EQ(directories_count, url.GetPathDepth());
}

TEST(Url, Util)
{
    std::string url_string = "http://www.qq.com/中文/";
    std::string encode_url_string = web::url::Url::EncodeUrl(url_string);
    EXPECT_EQ("http://www.qq.com/%D6%D0%CE%C4/", encode_url_string);
    EXPECT_EQ(url_string, web::url::Url::DecodeUrl(encode_url_string));

    std::string domain = "abc.sample.com:8080";
    EXPECT_EQ("com.sample.abc#8080", web::url::Url::ReverseDomain(domain));
    char reversed_domain[50];
    EXPECT_STREQ("com.sample.abc#8080",
                 web::url::Url::ReverseDomain(domain.c_str(), domain.length(), reversed_domain));
}

TEST(Url, GetHash)
{
    web::url::Url url;
    std::string url_string;

    url_string = "http://www.qq.com:8080/x/y/z/query.php?a=1";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));

    EXPECT_EQ(10046488241106336461llu, url.GetUrlHash());
    EXPECT_EQ(2796452991012973725llu, url.GetHostHash());
    EXPECT_EQ(15563144452842189574llu, url.GetHostPortHash());
    EXPECT_EQ(5880770708274948043llu, url.GetPortalPathHash());
    EXPECT_EQ(16020322104173420908llu, url.GetReversedSegmentPathHash());
}

TEST(Load, Success)
{
    web::url::Url url;
    std::string url_string;

    url_string = "www.qq.com:65535";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length(), web::url::E_HTTP_PROTOCOL));
    EXPECT_EQ("http://www.qq.com:65535/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com:65535/", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www#65535\"http\"/", url.GetReversedUrl());

    url_string = "http://127.0.0.1";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://127.0.0.1/", url.GetAbsolutePath());

    url_string = "http://user:password@sample:65535/";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://sample:65535/", url.GetAbsolutePath());

    url_string = "http://www.qq.com...:65535";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com:65535/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com:65535/", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www#65535\"http\"/", url.GetReversedUrl());

    url_string = "http://www.QQ.com/x/Y/z/query.php?c=3&C=33&b=&b=2&D&a=1&";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/x/Y/z/query.php?a=1&b=2&C=33&c=3&D", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/x/y/z/query.php?a=1&b=2&c=3&d", url.GetNormalizedUrl());
    EXPECT_EQ("/x/Y/z/query.php?a=1&b=2&C=33&c=3&D", url.GetRelativePath());
    EXPECT_EQ("query.php", url.GetResource());
    EXPECT_EQ("php", url.GetResourceSuffix());

    web::url::Url url_obj(url_string.c_str(), url_string.length());
    EXPECT_TRUE(url_obj.IsValid());
}

TEST(Load, Fail)
{
    web::url::Url url;
    std::string url_string;

    url_string = "http:///";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));

    url_string = "http://...";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));

    url_string = "http://sample:/";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));

    url_string = "http://sample:port/";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));

    url_string = "http://user:password@sample:port/";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));

    url_string = "http://sample:123456/";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));

    url_string = "http://sample:-1/";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));

    url_string = "http://sample:65536/";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));

    url_string = "http://user:password@sample:65536/";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));

    web::url::Url url_obj(url_string.c_str(), url_string.length());
    EXPECT_FALSE(url_obj.IsValid());
}

TEST(CombineLoad, Success)
{
    web::url::Url url, base_url;
    std::string url_string;
    std::string base_url_string;

    base_url_string = "http://www.qq.com";
    url_string = ".././../../?g&E=&f&f=&f=6";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length(),
                         base_url_string.c_str(), base_url_string.length()));
    EXPECT_EQ("http://www.qq.com/?E=&f=6&g", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/?e=&f=6&g", url.GetNormalizedUrl());

    EXPECT_TRUE(base_url.Load(base_url_string.c_str(), base_url_string.length()));
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length(), base_url));
    EXPECT_EQ("http://www.qq.com/?E=&f=6&g", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/?e=&f=6&g", url.GetNormalizedUrl());

    web::url::Url url_obj_1(url_string.c_str(), url_string.length(),
                            base_url_string.c_str(), base_url_string.length());
    EXPECT_TRUE(url_obj_1.IsValid());
    EXPECT_EQ("http://www.qq.com/?E=&f=6&g", url_obj_1.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/?e=&f=6&g", url_obj_1.GetNormalizedUrl());

    web::url::Url url_obj_2(url_string.c_str(), url_string.length(), base_url);
    EXPECT_TRUE(url_obj_2.IsValid());
    EXPECT_EQ("http://www.qq.com/?E=&f=6&g", url_obj_2.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/?e=&f=6&g", url_obj_2.GetNormalizedUrl());

    base_url_string = "http:///\\//www.qq.com///";
    url_string = "///\\/www.soso.com///?g&E=&f&f=&f=6";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length(),
                         base_url_string.c_str(), base_url_string.length()));
    EXPECT_EQ("http://www.soso.com/?E=&f=6&g", url.GetAbsolutePath());
}

TEST(CombineLoad, Fail)
{
    web::url::Url url;
    std::string url_string;
    std::string base_url_string;

    base_url_string = "www.qq.com";
    url_string = ".././../../?g&E=&f&f=&f=6";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length(),
                          base_url_string.c_str(), base_url_string.length()));

    web::url::Url url_obj(url_string.c_str(), url_string.length(),
                          base_url_string.c_str(), base_url_string.length());
    EXPECT_FALSE(url_obj.IsValid());
}

TEST(Url, LoadReversedUrl)
{
    web::url::Url url;
    std::string url_string;

    url_string = "com.error.www\\\"http\\\"/";
    EXPECT_FALSE(url.LoadReversedUrl(url_string.c_str(), url_string.length()));

    url_string = "com.qq.www\"http\"";
    EXPECT_TRUE(url.LoadReversedUrl(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/", url.GetAbsolutePath());

    url_string = "com.qq.www#65535";
    EXPECT_FALSE(url.LoadReversedUrl(url_string.c_str(), url_string.length()));
    EXPECT_TRUE(url.LoadReversedUrl(url_string.c_str(), url_string.length(),
                                    web::url::E_HTTP_PROTOCOL));
    EXPECT_EQ("http://www.qq.com:65535/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com:65535/", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www#65535\"http\"/", url.GetReversedUrl());
    EXPECT_FALSE(url.LoadReversedUrl(url_string.c_str(), url_string.length(),
                                     web::url::E_FTP_PROTOCOL));

    url_string = "com.qq.www\"http\"/";
    EXPECT_TRUE(url.LoadReversedUrl(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www\"http\"/", url.GetReversedUrl());

    url_string = "com.qq.www\"http\"/x/Y/z/";
    EXPECT_TRUE(url.LoadReversedUrl(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/x/Y/z/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/x/y/z", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www\"http\"/x/y/z", url.GetReversedUrl());

    url_string = "com.qq.www#65535\"http\"/x/Y/z/?E=&f=6&g";
    EXPECT_TRUE(url.LoadReversedUrl(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com:65535/x/Y/z/?E=&f=6&g", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com:65535/x/y/z/?e=&f=6&g", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www#65535\"http\"/x/y/z/?e=&f=6&g", url.GetReversedUrl());
}

TEST(Url, GetNormalizedUrl)
{
    web::url::Url url;
    std::string url_string;

    url_string = "http://www.QQ.com/x/Y/z/query.php?c=3&C=33&b=&b=2&D&a=1&";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/x/Y/z/query.php?a=1&b=2&C=33&c=3&D", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/x/y/z/query.php?a=1&b=2&c=3&d", url.GetNormalizedUrl());

    url_string = "http://www.QQ.com/x/Y/";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/x/Y/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/x/Y/", url.GetNormalizedUrl(web::url::NORMALIZE_FOR_DOWNLOAD));
    EXPECT_EQ("http://www.qq.com/x/Y/", url.GetNormalizedUrl(web::url::NORMALIZE_FOR_INDEX));
    EXPECT_EQ("http://www.qq.com/x/y", url.GetNormalizedUrl(web::url::NORMALIZE_FOR_DOCID));

    url_string = "http://www.QQ.com/index.html";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length(),
                         web::url::E_NO_PROTOCOL, web::url::NORMALIZE_FOR_DOWNLOAD));
    EXPECT_EQ("http://www.qq.com/index.html", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/index.html",
              url.GetNormalizedUrl(web::url::NORMALIZE_FOR_DOWNLOAD));
    EXPECT_EQ("http://www.qq.com/", url.GetNormalizedUrl(web::url::NORMALIZE_FOR_INDEX));
    EXPECT_EQ("http://www.qq.com/", url.GetNormalizedUrl(web::url::NORMALIZE_FOR_DOCID));
}

TEST(Url, GetReversedUrl)
{
    web::url::Url url;
    std::string url_string;

    url_string = "http://www.QQ.com/x/Y/z/query.php?c=3&C=33&b=&b=2&D&a=1&";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/x/Y/z/query.php?a=1&b=2&C=33&c=3&D", url.GetAbsolutePath());
    EXPECT_EQ("com.qq.www\"http\"/x/y/z/query.php?a=1&b=2&c=3&d", url.GetReversedUrl());

    url_string = "http://www.QQ.com/x/Y/";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/x/Y/", url.GetAbsolutePath());
    EXPECT_EQ("com.qq.www\"http\"/x/Y/", url.GetReversedUrl(web::url::NORMALIZE_FOR_DOWNLOAD));
    EXPECT_EQ("com.qq.www\"http\"/x/Y/", url.GetReversedUrl(web::url::NORMALIZE_FOR_INDEX));
    EXPECT_EQ("com.qq.www\"http\"/x/y", url.GetReversedUrl(web::url::NORMALIZE_FOR_DOCID));

    url_string = "http://www.QQ.com/index.html";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length(),
                         web::url::E_NO_PROTOCOL, web::url::NORMALIZE_FOR_DOWNLOAD));
    EXPECT_EQ("http://www.qq.com/index.html", url.GetAbsolutePath());
    EXPECT_EQ("com.qq.www\"http\"/index.html",
              url.GetReversedUrl(web::url::NORMALIZE_FOR_DOWNLOAD));
    EXPECT_EQ("com.qq.www\"http\"/", url.GetReversedUrl(web::url::NORMALIZE_FOR_INDEX));
    EXPECT_EQ("com.qq.www\"http\"/", url.GetReversedUrl(web::url::NORMALIZE_FOR_DOCID));
}

TEST(Url, ZeroPort)
{
    web::url::Url url;
    std::string url_string;

    url_string = "http://www.qq.com:0/";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www\"http\"/", url.GetReversedUrl());
    EXPECT_EQ("www.qq.com", url.GetHostPort());
    EXPECT_EQ(80, url.GetPort());

    url_string = "com.qq.www#0\"http\"/";
    EXPECT_TRUE(url.LoadReversedUrl(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www\"http\"/", url.GetReversedUrl());
    EXPECT_EQ("www.qq.com", url.GetHostPort());
    EXPECT_EQ(80, url.GetPort());
}

TEST(Url, Schemes)
{
    web::url::Url url;
    std::string url_string;

    url_string = "ftp://www.qq.com";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_FALSE(url.IsValid());
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length(),
                         web::url::E_NO_PROTOCOL, web::url::NORMALIZE_FOR_DOWNLOAD,
                         web::url::VALID_SCHEMES_FOR_DOWNLOAD));
    EXPECT_TRUE(url.IsValid());
    EXPECT_EQ("ftp://www.qq.com/", url.GetAbsolutePath());
    EXPECT_EQ("ftp://www.qq.com/", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www\"ftp\"/", url.GetReversedUrl());
    EXPECT_EQ("www.qq.com", url.GetHostPort());
    EXPECT_EQ(21, url.GetPort());

    url_string = "www.qq.com:65535";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length(),
                          web::url::E_FTP_PROTOCOL));
    EXPECT_FALSE(url.IsValid());
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length(),
                         web::url::E_FTP_PROTOCOL, web::url::NORMALIZE_FOR_DOWNLOAD,
                         web::url::VALID_SCHEMES_FOR_DOWNLOAD));
    EXPECT_TRUE(url.IsValid());
    EXPECT_EQ("ftp://www.qq.com:65535/", url.GetAbsolutePath());
    EXPECT_EQ("ftp://www.qq.com:65535/", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www#65535\"ftp\"/", url.GetReversedUrl());
    EXPECT_EQ("www.qq.com:65535", url.GetHostPort());
    EXPECT_EQ(65535, url.GetPort());

    url_string = "tencent://www.qq.com/";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length(),
                          web::url::E_HTTP_PROTOCOL));
    EXPECT_FALSE(url.IsValid());
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length(),
                          web::url::E_HTTP_PROTOCOL, web::url::NORMALIZE_FOR_DOWNLOAD,
                          web::url::VALID_SCHEMES_FOR_DOWNLOAD));
    EXPECT_FALSE(url.IsValid());

    url_string = "ftp://www.qq.com:65535";
    web::url::Url url_obj_1(url_string.c_str(), url_string.length());
    EXPECT_FALSE(url_obj_1.IsValid());
    web::url::Url url_obj_2(url_string.c_str(), url_string.length(),
                            web::url::E_NO_PROTOCOL, web::url::NORMALIZE_FOR_DOWNLOAD,
                            web::url::VALID_SCHEMES_FOR_DOWNLOAD);
    EXPECT_TRUE(url_obj_2.IsValid());
    EXPECT_EQ("ftp://www.qq.com:65535/", url_obj_2.GetAbsolutePath());
    EXPECT_EQ("ftp://www.qq.com:65535/", url_obj_2.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www#65535\"ftp\"/", url_obj_2.GetReversedUrl());
    EXPECT_EQ("www.qq.com:65535", url_obj_2.GetHostPort());
    EXPECT_EQ(65535, url_obj_2.GetPort());
}

TEST(Url, Query)
{
    web::url::Url url;
    std::string url_string;

    url_string = "http://www.qq.com/?c=3&中文=值&C=33&&%D6%D0%CE%C4=6&b=& &b=2&%61&D& a = 1 &";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length(),
                         web::url::E_HTTP_PROTOCOL, web::url::NORMALIZE_FOR_DOWNLOAD));
    EXPECT_TRUE(url.IsValid());
    EXPECT_EQ("http://www.qq.com/?c=3&中文=值&C=33&%D6%D0%CE%C4=6&b=&b=2&%61&D&a=1",
              url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/?a=1&b=2&c=3&d&中文=值", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www\"http\"/?a=1&b=2&c=3&d&中文=值", url.GetReversedUrl());

    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_TRUE(url.IsValid());
    EXPECT_EQ("http://www.qq.com/?%61&%D6%D0%CE%C4=6&a=1&b=2&C=33&c=3&D&中文=值",
              url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/?a=1&b=2&c=3&d&中文=值", url.GetNormalizedUrl());
    EXPECT_EQ("com.qq.www\"http\"/?a=1&b=2&c=3&d&中文=值", url.GetReversedUrl());

    std::map<std::string, std::string> querys;
    url.GetQuerys(querys);
    uint32_t querys_count = 8;
    ASSERT_EQ(querys_count, querys.size());
    EXPECT_EQ("1", querys["a"]);
    EXPECT_EQ("2", querys["b"]);
    EXPECT_EQ("3", querys["c"]);
    EXPECT_EQ("33", querys["C"]);
    EXPECT_EQ("", querys["D"]);
    EXPECT_EQ("", querys["%61"]);
    EXPECT_EQ("6", querys["%D6%D0%CE%C4"]);
    EXPECT_EQ("值", querys["中文"]);
}

TEST(Load, TrimWhiteSpace)
{
    web::url::Url url;
    std::string url_string;

    url_string = "  ";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));

    url_string = " \" \" ";
    EXPECT_FALSE(url.Load(url_string.c_str(), url_string.length()));

    url_string = " http://www.qq.com/ ";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/", url.GetNormalizedUrl());

    url_string = "http://www.qq.com/%20";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/", url.GetNormalizedUrl());

    url_string = "  \"%20%20http://www.qq.com/  \" %20 %20 ";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/", url.GetNormalizedUrl());

    url_string = "%20http://www.qq.com/%20dir/%20";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/%20dir/", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/ dir", url.GetNormalizedUrl());

    url_string = "http://www.qq.com/%20%41";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_EQ("http://www.qq.com/%20%41", url.GetAbsolutePath());
    EXPECT_EQ("http://www.qq.com/ a", url.GetNormalizedUrl());
}

TEST(Url, Clear)
{
    web::url::Url url;
    std::string url_string;

    url_string = "http://www.qq.com/";
    EXPECT_TRUE(url.Load(url_string.c_str(), url_string.length()));
    EXPECT_TRUE(url.IsValid());

    url.Clear();
    EXPECT_FALSE(url.IsValid());
    EXPECT_EQ("", url.GetNormalizedUrl());
    EXPECT_EQ("", url.GetReversedUrl());
    EXPECT_EQ("", url.GetAbsolutePath());
    EXPECT_STREQ("", url.GetAbsolutePathPtr());
    EXPECT_EQ("", url.GetRelativePath());
    EXPECT_EQ(NULL, url.GetRelativePathPtr());
    EXPECT_EQ("", url.GetPortalPath());
    EXPECT_EQ("", url.GetSegmentPath());
    EXPECT_EQ("", url.GetReversedSegmentPath());
    EXPECT_EQ("", url.GetHomePageURL());
    EXPECT_EQ(web::url::E_NO_PROTOCOL, url.GetScheme());
    EXPECT_EQ("", url.GetSchemeString());
    EXPECT_EQ("", url.GetHost());
    EXPECT_STREQ("", url.GetHostPtr());
    EXPECT_EQ("", url.GetHostPort());
    EXPECT_EQ("", url.GetReversedHost());
    EXPECT_EQ("", url.GetReversedHostPort());
    EXPECT_EQ(0, url.GetPort());
    EXPECT_EQ("", url.GetPath());
    EXPECT_STREQ("", url.GetPathPtr());
    std::map<std::string, std::string> querys;
    url.GetQuerys(querys);
    uint32_t querys_count = 0;
    ASSERT_EQ(querys_count, querys.size());
    EXPECT_EQ("", url.GetDirectory());
    std::vector<std::string> directories;
    url.GetDirectories(directories);
    uint32_t directories_count = 0;
    ASSERT_EQ(directories_count, directories.size());
    EXPECT_EQ("", url.GetResource());
    EXPECT_EQ(NULL, url.GetResourcePtr());
    EXPECT_EQ("", url.GetResourceSuffix());
    EXPECT_EQ(NULL, url.GetResourceSuffixPtr());
    EXPECT_EQ(directories_count, url.GetPathDepth());
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
