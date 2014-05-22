// Copyright (c) 2011, Tencent Inc. All rights reserved.
// Author:  Wen Jie <welkinwen@tencent.com>
// Created: 2011-12-20

#include "common/web/docid_calc.h"
#include "common/crypto/hash/md5.h"

namespace web {

void DocidCalc::CalcDocid(const web::url::Url& url, Md5HashValue* docid)
{
    if (url.IsValid())
    {
        uint64_t url_hash = common::MD5::Digest64(
            url.GetNormalizedUrl(web::url::NORMALIZE_FOR_DOCID).c_str());
        uint64_t host_port_hash = common::MD5::Digest64(url.GetHostPort().c_str());

        docid->SetValue(&url_hash, sizeof(url_hash), 0);
        docid->SetValue(&host_port_hash, sizeof(host_port_hash), sizeof(url_hash));
    }
    else
    {
        docid->Clear();
    }
}

void DocidCalc::CalcDocid(const web::url::Url& url, Md5HashValue96* docid)
{
    if (url.IsValid())
    {
        Md5HashValue md5;
        common::MD5::Digest(
            url.GetNormalizedUrl(web::url::NORMALIZE_FOR_DOCID), md5.AsBytes());

        docid->SetValue(md5.AsBytes());
    }
    else
    {
        docid->Clear();
    }
}

void DocidCalc::CalcDocid(const web::url::Url& url, uint64_t* docid)
{
    if (url.IsValid())
    {
        *docid = common::MD5::Digest64(
            url.GetNormalizedUrl(web::url::NORMALIZE_FOR_DOCID));
    }
    else
    {
        *docid = 0;
    }
}

void DocidCalc::CalcDocid(const std::string& url_string, Md5HashValue* docid)
{
    web::url::Url url(url_string.c_str(), url_string.length());
    CalcDocid(url, docid);
}

void DocidCalc::CalcDocid(const std::string& url_string, Md5HashValue96* docid)
{
    web::url::Url url(url_string.c_str(), url_string.length());
    CalcDocid(url, docid);
}

void DocidCalc::CalcDocid(const std::string& url_string, uint64_t* docid)
{
    web::url::Url url(url_string.c_str(), url_string.length());
    CalcDocid(url, docid);
}

void DocidCalc::CalcDocidOfReversedUrl(const std::string& reversed_url, Md5HashValue* docid)
{
    web::url::Url url;
    url.LoadReversedUrl(reversed_url.c_str(), reversed_url.length());
    CalcDocid(url, docid);
}

void DocidCalc::CalcDocidOfReversedUrl(const std::string& reversed_url, Md5HashValue96* docid)
{
    web::url::Url url;
    url.LoadReversedUrl(reversed_url.c_str(), reversed_url.length());
    CalcDocid(url, docid);
}

void DocidCalc::CalcDocidOfReversedUrl(const std::string& reversed_url, uint64_t* docid)
{
    web::url::Url url;
    url.LoadReversedUrl(reversed_url.c_str(), reversed_url.length());
    CalcDocid(url, docid);
}

Md5HashValue DocidCalc::CalcDocid128bit(const web::url::Url & url)
{
    Md5HashValue docid;
    CalcDocid(url, &docid);
    return docid;
}

Md5HashValue DocidCalc::CalcDocid128bit(const std::string & url_string)
{
    Md5HashValue docid;
    CalcDocid(url_string, &docid);
    return docid;
}

Md5HashValue DocidCalc::CalcDocid128bitOfReversedUrl(const std::string & reversed_url)
{
    Md5HashValue docid;
    CalcDocidOfReversedUrl(reversed_url, &docid);
    return docid;
}

Md5HashValue96 DocidCalc::CalcDocid96bit(const web::url::Url & url)
{
    Md5HashValue96 docid;
    CalcDocid(url, &docid);
    return docid;
}

Md5HashValue96 DocidCalc::CalcDocid96bit(const std::string & url_string)
{
    Md5HashValue96 docid;
    CalcDocid(url_string, &docid);
    return docid;
}

Md5HashValue96 DocidCalc::CalcDocid96bitOfReversedUrl(const std::string & reversed_url)
{
    Md5HashValue96 docid;
    CalcDocidOfReversedUrl(reversed_url, &docid);
    return docid;
}

uint64_t DocidCalc::CalcDocid64bit(const web::url::Url & url)
{
    uint64_t docid;
    CalcDocid(url, &docid);
    return docid;
}

uint64_t DocidCalc::CalcDocid64bit(const std::string & url_string)
{
    uint64_t docid;
    CalcDocid(url_string, &docid);
    return docid;
}

uint64_t DocidCalc::CalcDocid64bitOfReversedUrl(const std::string & reversed_url)
{
    uint64_t docid;
    CalcDocidOfReversedUrl(reversed_url, &docid);
    return docid;
}

void DocidCalc::SplitDocid(const Md5HashValue& docid_128bit,
                           uint64_t* docid_64bit, uint64_t* hostportid_64bit)
{
    memcpy(docid_64bit, docid_128bit.AsBytes(), sizeof(*docid_64bit));
    memcpy(hostportid_64bit, docid_128bit.AsBytes() + sizeof(*docid_64bit),
           sizeof(*hostportid_64bit));
}

} // namespace web
