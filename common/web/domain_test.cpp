#define USE_MD5 1

#include <fstream>
#include <iostream>
#include <map>
#include "common/crypto/hash/md5.h"
#define private public
#include "common/web/domain.h"
#undef private
#include "thirdparty/gtest/gtest.h"

using namespace std;
using namespace websearch::domain;

TEST(Domain, Initialize)
{
    EXPECT_TRUE(Domain::Initialize("domainsuffix.txt", "blogsuffix.txt", NULL));

    // EXPECT_FALSE(Domain::Initialize(true, true, "domain.boxerzhang.com"));
    // EXPECT_TRUE(Domain::Initialize(true, true, "domainserver.twse.spider.all.soso-idc.com"));
    // EXPECT_FALSE(websearch::domain::Domain::Initialize(true, true, "qq.com"));
}

TEST(Domain, GetDomainSuffix)
{
    EXPECT_TRUE(Domain::Initialize("domainsuffix.txt", NULL, NULL));
    {
        Domain domain("news.qq.com");
        ASSERT_EQ(std::string(domain.GetDomainSuffix()), std::string(".com"));
    }
    {
        Domain domain("news.sina.com.cn");
        ASSERT_EQ(std::string(domain.GetDomainSuffix()), std::string(".com.cn"));
    }
    {
        Domain domain("news.djq");
        ASSERT_TRUE(domain.GetDomainSuffix() == NULL);
    }
    {
        Domain domain("news.qq.com:123");
        EXPECT_TRUE(domain.GetDomainSuffix() == std::string(".com"));
    }
}

TEST(Domain, GetDomain)
{
    EXPECT_TRUE(Domain::Initialize("domainsuffix.txt", NULL, NULL));
    {
        Domain domain("219.238.187.126:9053");
        EXPECT_EQ("219.238.187.126", std::string(domain.GetDomain()));
    }
    {
        Domain domain("219.238.187.126:90531");
        EXPECT_EQ("", std::string(domain.GetDomain()));
    }
    {
        Domain domain("news.qq.com");
        EXPECT_EQ("qq.com", std::string(domain.GetDomain()));
    }
    {
        Domain domain("news.Qq.cOM");
        EXPECT_EQ("qq.com", std::string(domain.GetDomain()));
    }
    {
        Domain domain("abcd.info");
        EXPECT_EQ("abcd.info", std::string(domain.GetDomain()));
    }
    {
        Domain domain("news.sina.com.cn");
        EXPECT_EQ("sina.com.cn", std::string(domain.GetDomain()));
    }
    {
        Domain domain("news.djq");
        EXPECT_EQ("", std::string(domain.GetDomain()));
    }
    {
        Domain domain("127.0.0.1");
        EXPECT_EQ("127.0.0.1", std::string(domain.GetDomain()));
    }
    {
        Domain domain("127.0.0.1:80");
        EXPECT_EQ("127.0.0.1", std::string(domain.GetDomain()));
    }
    {
        Domain domain("219.238.187.126:9053");
        EXPECT_TRUE(domain.IsValidDomain());
    }
    {
        Domain domain("67.220.92.00000000000017");
        EXPECT_TRUE(domain.IsValidDomain());
    }
    {
        Domain domain("67.220.92.17a");
        EXPECT_FALSE(domain.IsValidDomain());
    }
    {
        Domain domain("news.qq.com:80");
        EXPECT_EQ("qq.com", std::string(domain.GetDomain()));
    }
    {
        Domain domain("web.medkaoyan.net");
        EXPECT_EQ("medkaoyan.net", std::string(domain.GetDomain()));
        EXPECT_EQ(".net", std::string(domain.GetDomainSuffix()));
    }
    {
        Domain domain("67.220.92.000000000000000000000000000000000000000017");
        EXPECT_FALSE(domain.IsValidDomain());
    }
}

TEST(Domain, IsValidDomain)
{
    { Domain domain("news.qq.com:123"); ASSERT_EQ(domain.IsValidDomain(), true); }
    { Domain domain(".QQ.COM"); ASSERT_EQ(domain.IsValidDomain(), false); }
    { Domain domain("news.djq"); ASSERT_EQ(domain.IsValidDomain(), false); }
    { Domain domain("news.djq:123"); ASSERT_EQ(domain.IsValidDomain(), false); }
    { Domain domain("NEWS.QQ.COM"); ASSERT_EQ(domain.IsValidDomain(), true); }
    { Domain domain("news.qq.com"); ASSERT_EQ(domain.IsValidDomain(), true); }
    { Domain domain("www.qsxiaoshuo.com"); ASSERT_EQ(domain.IsValidDomain(), true); }

    { Domain domain("172.0.0.1"); ASSERT_EQ(domain.IsValidDomain(), true); }
    { Domain domain("0.0.0.0"); ASSERT_EQ(domain.IsValidDomain(), true); }
    { Domain domain("172.0.0.1:65536"); ASSERT_EQ(domain.IsValidDomain(), false); }
    { Domain domain("172.0.0.1:65535"); ASSERT_EQ(domain.IsValidDomain(), true); }
    { Domain domain("10.11.1"); ASSERT_EQ(domain.IsValidDomain(), false); }
    { Domain domain("10.11.1.1.1"); ASSERT_EQ(domain.IsValidDomain(), false); }
    { Domain domain("10.11.1.256"); ASSERT_EQ(domain.IsValidDomain(), false); }

    // 连续的分隔符号
    { Domain domain("news..qq.com"); ASSERT_EQ(domain.IsValidDomain(), false); }
    { Domain domain("news.qq.com::100"); ASSERT_EQ(domain.IsValidDomain(), false); }

    // . 过多
    { Domain domain("1.2.3.4.5.com"); ASSERT_EQ(domain.IsValidDomain(), true); }
    { Domain domain("1.2.3.4.5.6.7.com"); ASSERT_EQ(domain.IsValidDomain(), false); }

    // 端口后面又出现了字母
    { Domain domain("www.qq.com:123a"); ASSERT_EQ(domain.IsValidDomain(), false); }
    { Domain domain("www.qq.com:123a1"); ASSERT_EQ(domain.IsValidDomain(), false); }


    { Domain domain(".com"); ASSERT_EQ(domain.IsValidDomain(), false); }
    { Domain domain("com"); ASSERT_EQ(domain.IsValidDomain(), false); }
    { Domain domain("255.255.255.255:65535"); EXPECT_TRUE(domain.IsValidDomain()); }
    { Domain domain("255.255.255.255:65536"); EXPECT_FALSE(domain.IsValidDomain()); }
    { Domain domain("?Ž?ǿ.com"); EXPECT_FALSE(domain.IsValidDomain()); }
    { Domain domain(""); EXPECT_FALSE(domain.IsValidDomain()); }
    { Domain domain(NULL); EXPECT_FALSE(domain.IsValidDomain()); }
    {
        char p[10];
        p[0] = '\0';
        Domain domain(p);
        EXPECT_FALSE(domain.IsValidDomain());
    }

    {
        // ?ܹ? 49 ???ַ?
        const char* p = "012345678901234567890123456789012345678901234.com";
        EXPECT_EQ(strlen(p), 49ull);

        Domain domain(p);
        EXPECT_TRUE(domain.IsValidDomain());
    }
    {
        // ?ܹ? 50 ???ַ?
        const char* p = "0123456789012345678901234567890123456789012345.com";
        EXPECT_EQ(strlen(p), 50ull);

        Domain domain(p);
        EXPECT_TRUE(domain.IsValidDomain());
    }
    {
        // ?ܹ? 51 ???ַ?
        const char* p = "01234567890123456789012345678901234567890123456.com";
        EXPECT_EQ(strlen(p), 51ull);

        Domain domain(p);
        EXPECT_FALSE(domain.IsValidDomain());
    }
    {
        // 50 ???ַ??????˿?
        const char* p = "0123456789012345678901234567890123456789012345.com:65535";
        EXPECT_EQ(strlen(p), 56ull);

        Domain domain(p);
        EXPECT_TRUE(domain.IsValidDomain());
    }
    {
        Domain domain("202011.114919.201712.144826.129471.hortonline.com");
        EXPECT_TRUE(domain.IsValidDomain());
        EXPECT_EQ(std::string("hortonline.com"), domain.GetDomain());
    }
    {
        Domain domain("sosospider.114919.201712.144826.129471.hortonline.com");
        EXPECT_FALSE(domain.IsValidDomain());
        EXPECT_EQ(std::string(""), domain.GetDomain());
    }
    {
        Domain domain("67.220.91.0017");
        EXPECT_TRUE(domain.IsValidDomain());
        EXPECT_EQ(std::string("67.220.91.0017"), domain.GetDomain());
    }
}

TEST(Domain, GetScheSubdomain)
{
    EXPECT_TRUE(Domain::Initialize("domainsuffix.txt", "blogsuffix.txt", NULL));
    {
        Domain domain("news.qq.com:123");
        char* p = NULL;
        bool bRet = domain.GetScheSubdomain(p);
        ASSERT_EQ(bRet, false);
        ASSERT_EQ(std::string("news.qq.com:123"), std::string(p));
    }

    {
        Domain domain("news.0-6.com");
        char* p = NULL;
        bool bRet = domain.GetScheSubdomain(p);
        ASSERT_EQ(bRet, true);
        ASSERT_EQ(std::string("sosospider.0-6.com"), std::string(p));
    }
    {
        Domain domain("www.0-6.com");
        char* p = NULL;
        bool bRet = domain.GetScheSubdomain(p);
        ASSERT_EQ(bRet, false);
        ASSERT_EQ(std::string("www.0-6.com"), std::string(p));
    }
    {
        Domain domain("bj.1998.cn");
        char* p = NULL;
        bool bRet = domain.GetScheSubdomain(p);
        ASSERT_EQ(bRet, false);
        ASSERT_EQ(std::string("bj.1998.cn"), std::string(p));
    }
    {
        Domain domain("djq.1998.cn");
        char* p = NULL;
        bool bRet = domain.GetScheSubdomain(p);
        ASSERT_EQ(bRet, true);
        ASSERT_EQ(std::string("sosospider.1998.cn"), std::string(p));
    }
    {
        Domain domain("com.multiple_select_1");
        char* p = NULL;
        bool bRet = domain.GetScheSubdomain(p);
        ASSERT_EQ(bRet, false);
        ASSERT_EQ(std::string("com.multiple_select_1"), std::string(p));
    }
}

TEST(Domain, Performance)
{
    EXPECT_TRUE(Domain::Initialize("domainsuffix.txt", "blogsuffix.txt", NULL));
    ifstream file1("domain_test.txt");
    string line;

    vector<string> vec;
    while (getline(file1, line))
    {
        vec.push_back(line);
    }

    time_t begin = time(0);
    for (size_t i = 0; i < vec.size(); i++)
    {
        Domain domain(vec[i].c_str());
        domain.GetDomain();
    }
    time_t end = time(0);
    float speed = 0;
    if (end - begin != 0)
    {
        speed = vec.size() / (end - begin);
        cout << "load domain " << vec.size() << ", GetDomain speed: " << speed << endl;
    }

    begin = time(0);
    for (size_t i = 0; i < vec.size(); i++)
    {
        char* p = NULL;
        Domain domain(vec[i].c_str());
        domain.GetScheSubdomain(p);
    }
    end = time(0);
    if (end - begin != 0)
    {
        speed = vec.size() / (end - begin);
        cout << "load domain " << vec.size() << ", GetScheSubdomain speed: " << speed << endl;
    }
}

TEST(Domain, DISABLED_GetDataFromServer)
{
    EXPECT_TRUE(Domain::Initialize(true, true));
    {
        Domain domain("news.0-6.com");
        char* p = NULL;
        bool bRet = domain.GetScheSubdomain(p);
        ASSERT_EQ(bRet, true);
        ASSERT_EQ(std::string("sosospider.0-6.com"), std::string(p));
    }
    {
        Domain domain("www.0-6.com");
        char* p = NULL;
        bool bRet = domain.GetScheSubdomain(p);
        ASSERT_EQ(bRet, false);
        ASSERT_EQ(std::string("www.0-6.com"), std::string(p));
    }
    {
        Domain domain("sosospider.0-6.com");
        char* p = NULL;
        bool bRet = domain.GetScheSubdomain(p);
        ASSERT_EQ(bRet, true);
        ASSERT_EQ(std::string("sosospider.0-6.com"), std::string(p));
    }
    {
        Domain domain("news.qq.com");
        char* p = NULL;
        bool bRet = domain.GetRelScheSubdomain(p);
        ASSERT_EQ(bRet, false);
        ASSERT_EQ(std::string("news.qq.com"), std::string(p));
    }
    {
        Domain domain("alexdu.blog.sohu.com");
        char* p = NULL;
        bool bRet = domain.GetRelScheSubdomain(p);
        ASSERT_EQ(bRet, true);
        ASSERT_EQ(std::string(".blog.sohu.com"), std::string(p));
    }
    {
        Domain domain("alexdu.sina.com.cn");
        char* p = NULL;
        bool bRet = domain.GetRelScheSubdomain(p);
        ASSERT_EQ(bRet, false);
        ASSERT_EQ(std::string("alexdu.sina.com.cn"), std::string(p));
    }
}

TEST(Domain, GetReverseSubDomain)
{
    char dest[1024];
    Domain::GetReverseSubDomain(dest, (char*)"news.qq.com");
    ASSERT_EQ(std::string("com.qq.news"), std::string(dest));

    Domain::GetReverseSubDomain(dest, (char*)"news.qq.com:80");
    ASSERT_EQ(std::string("com.qq.news:80"), std::string(dest));

    Domain::GetReverseSubDomain(dest, (char*)"news.qq.com:80:80");
    ASSERT_EQ(std::string("com.qq.news:80:80"), std::string(dest));

    Domain::GetReverseSubDomain(dest, (char*)"news.qq.com:90/1.html", 14);
    ASSERT_EQ(std::string("com.qq.news:90"), std::string(dest));

    Domain::GetReverseSubDomain(dest, (char*)"this.is.a.test");
    ASSERT_EQ(std::string("test.a.is.this"), std::string(dest));

    Domain::GetReverseSubDomain(dest, (char*)"0.0.1.0001");
    ASSERT_EQ(std::string("0001.1.0.0"), std::string(dest));
}

TEST(Domain, DISABLED_Reload)
{
    //EXPECT_TRUE(Domain::Initialize(true, true));

    unsigned int reload_times = 10;

    unsigned int begin = time(NULL);
    for (unsigned int i = 0; i < reload_times; i ++)
    {
        bool ret = Domain::Reload();
        ASSERT_TRUE(ret);
    }
    unsigned int end = time(NULL);
    cout << "avg reload speed: " << (end - begin) / reload_times << endl;
}


TEST(Domain, GetSubdomainID)
{
    {
        Domain domain("news.qq.com");
        unsigned long long sub_domain_id = domain.GetSubdomainID();
        EXPECT_EQ(sub_domain_id, 12993808137765405488ull);
    }
    {
        Domain domain("news.qq.abc");
        unsigned long long sub_domain_id = domain.GetSubdomainID();
        EXPECT_EQ(sub_domain_id, 0ull);
    }
}

TEST(Domain, GetDomainID)
{
    {
        Domain domain("news.qq.com");
        unsigned long long sub_domain_id = domain.GetDomainID();
        EXPECT_EQ(sub_domain_id, 16278419776137229227ull);
    }
    {
        Domain domain("news.qq.abc");
        unsigned long long sub_domain_id = domain.GetDomainID();
        EXPECT_EQ(sub_domain_id, 0ull);
    }
}

TEST(Domain, GetScheSubdomainID)
{
    EXPECT_TRUE(Domain::Initialize("domainsuffix.txt", "blogsuffix.txt", "rel_aggre_suffix.txt"));
    {
        const char* p = "news.qq.com:123";
        Domain domain(p);
        unsigned long long sche_subdomain_id = 0;
        EXPECT_FALSE(domain.GetScheSubdomainID(&sche_subdomain_id));

        unsigned long long expect_id = common::MD5::Digest64(p);
        EXPECT_EQ(expect_id, sche_subdomain_id);
    }

    {
        Domain domain("news.0-6.com");
        unsigned long long sche_subdomain_id = 0;
        EXPECT_TRUE(domain.GetScheSubdomainID(&sche_subdomain_id));
        EXPECT_EQ(14926434632578228459ull, sche_subdomain_id);
    }
}

TEST(Domain, GetRelScheSubdomainID)
{
    EXPECT_TRUE(Domain::Initialize("domainsuffix.txt", "blogsuffix.txt", "rel_aggre_suffix.txt"));
    {
        const char* p = "boxerzhang.sina.com.cn";
        Domain domain(p);
        unsigned long long sche_subdomain_id;
        EXPECT_FALSE(domain.GetRelScheSubdomainID(&sche_subdomain_id));

        unsigned long long expect_id = common::MD5::Digest64(p);
        EXPECT_EQ(expect_id, sche_subdomain_id);
    }
    {
        Domain domain("boxerzhang.blog.sohu.com");
        unsigned long long sche_subdomain_id;
        EXPECT_TRUE(domain.GetRelScheSubdomainID(&sche_subdomain_id));
        EXPECT_EQ(6107456793652757769ull, sche_subdomain_id);
    }
}

TEST(Domain, TryGetDomain)
{
    EXPECT_TRUE(Domain::Initialize("domainsuffix.txt", "blogsuffix.txt", "rel_aggre_suffix.txt"));
    char domain[1024];
    std::vector<std::string> wrong_subdomain;
    wrong_subdomain.push_back("");
    wrong_subdomain.push_back("com");
    wrong_subdomain.push_back(".");
    wrong_subdomain.push_back(".com");
    wrong_subdomain.push_back(".null");
    wrong_subdomain.push_back(".com.cn");
    wrong_subdomain.push_back("com.");
    wrong_subdomain.push_back(".com.");
    wrong_subdomain.push_back("..com");
    wrong_subdomain.push_back(".a..com");
    wrong_subdomain.push_back("#.com.cn");
    wrong_subdomain.push_back("10.6..1");
    wrong_subdomain.push_back(":10.6.2.4");
    wrong_subdomain.push_back("10.a.6.2.4");
    wrong_subdomain.push_back("256.0.0.1");
    wrong_subdomain.push_back("192.168.0.1a");
    wrong_subdomain.push_back("00000069.4.234.228");
    for (size_t i = 0; i < wrong_subdomain.size(); ++i)
    {
        memset(domain, '\0', 1024);
        EXPECT_FALSE(Domain::TryGetDomain(wrong_subdomain[i].data(),
                                       wrong_subdomain[i].size(),
                                       domain)) << wrong_subdomain[i];
    }

    std::map<std::string, std::string> subdomain;
    subdomain["qq.com"] = "qq.com";
    subdomain["www.qq.com:port"] = "qq.com";
    subdomain["*.qq.com"] = "qq.com";
    subdomain[".*.qq.com"] = "qq.com";
    subdomain["qq.com.cn"] = "qq.com.cn";
    subdomain["www.qq.com.cn"] = "qq.com.cn";
    subdomain["*.qq.com.cn"] = "qq.com.cn";
    subdomain["#^%a.qq.com"] = "qq.com";
    subdomain["255.255.255.255:2012"] = "255.255.255.255";
    subdomain["*.115.154.89.57"] = "115.154.89.57";
    subdomain["115.154.89.57"] = "115.154.89.57";
    subdomain["66.206.0.25"] = "66.206.0.25";
    subdomain["67.220.92.00000000000017"] = "67.220.92.00000000000017";
    std::map<std::string, std::string>::const_iterator it = subdomain.begin();
    for ( ; subdomain.end() != it; ++it)
    {
        memset(domain, '\0', 1024);
        bool ret = Domain::TryGetDomain(
            it->first.data(), it->first.size(), domain);
        EXPECT_TRUE(ret) << it->first;
        EXPECT_EQ(it->second, std::string(domain));
        memset(domain, '\0', 1024);
        EXPECT_TRUE(Domain::TryGetDomain(it->first.data(), domain)) << it->first;
    }
}

