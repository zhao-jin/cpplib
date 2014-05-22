// Copyright (c) 2011, Tencent Inc. All rights reserved.
// Author:  Wen Jie <welkinwen@tencent.com>
// Created: 2011-04-20

#include "common/web/url_util.h"
#include <stdio.h>

namespace web {
namespace url {

/// 获取反转形式的片段路径的父目录
/// @param  segment_path 反转形式的片段路径
/// @return 反转形式的片段路径的父目录（同为反转形式）
std::string UrlUtil::GetParentOfReversedSegmentPath(const std::string & segment_path)
{
    if (segment_path.length() > 1)
    {
        std::string::size_type pos = segment_path.rfind('/', segment_path.length() - 2);

        if (pos != std::string::npos)
        {
            return segment_path.substr(0, pos + 1);
        }
    }

    return std::string("");
}

/// 获取URL的所有目录前缀
/// @param  url      URL字符串
/// @param  prefixes 存放目录前缀的vector
/// @retval true     提取目录前缀成功
/// @retval false    URL无效，提取目录前缀失败
bool UrlUtil::GetPrefixesOfUrl(const std::string & url, std::vector<std::string> & prefixes)
{
    Url url_obj;

    if (url_obj.Load(url.c_str(), url.length()))
    {
        return UrlUtil::GetPrefixesOfUrl(url_obj, prefixes);
    }

    return false;
}

/// 获取URL的所有目录前缀
/// @param  url      Url对象
/// @param  prefixes 存放目录前缀的vector
/// @retval true     提取目录前缀成功
/// @retval false    Url对象无效，提取目录前缀失败
bool UrlUtil::GetPrefixesOfUrl(const Url & url, std::vector<std::string> & prefixes)
{
    if (url.IsValid())
    {
        std::string prefix = url.GetHomePageURL();
        prefix.resize(prefix.length() - 1);

        std::vector<std::string> directories;
        url.GetDirectories(directories);

        for (size_t i = 0; i < directories.size(); ++i)
        {
            prefixes.push_back(prefix + directories[i]);
        }

        return true;
    }

    return false;
}

/// 获取URL的所有（反转）目录前缀
/// @param  url               URL字符串
/// @param  reversed_prefixes 存放（反转）目录前缀的vector
/// @retval true              提取（反转）目录前缀成功
/// @retval false             URL无效，提取（反转）目录前缀失败
bool UrlUtil::GetReversedPrefixesOfUrl(const std::string & url,
    std::vector<std::string> & reversed_prefixes)
{
    Url url_obj;

    if (url_obj.Load(url.c_str(), url.length()))
    {
        return UrlUtil::GetReversedPrefixesOfUrl(url_obj, reversed_prefixes);
    }

    return false;
}

/// 获取URL的所有（反转）目录前缀
/// @param  url               Url对象
/// @param  reversed_prefixes 存放（反转）目录前缀的vector
/// @retval true              提取（反转）目录前缀成功
/// @retval false             Url对象无效，提取（反转）目录前缀失败
bool UrlUtil::GetReversedPrefixesOfUrl(const Url & url,
    std::vector<std::string> & reversed_prefixes)
{
    if (url.IsValid())
    {
        std::string reversed_url = url.GetReversedUrl(0);
        std::string::size_type pos = reversed_url.find('"');
        if (pos == std::string::npos)
        {
            return false;
        }
        pos = reversed_url.find('"', pos + 1);
        if (pos == std::string::npos)
        {
            return false;
        }
        std::string reversed_prefix = reversed_url.substr(0, pos + 1);

        std::vector<std::string> directories;
        url.GetDirectories(directories);

        for (size_t i = 0; i < directories.size(); ++i)
        {
            reversed_prefixes.push_back(reversed_prefix + directories[i]);
        }

        return true;
    }

    return false;
}

/// 获取（反转）URL的所有目录前缀
/// @param  reversed_url  （反转）URL的字符串
/// @param  prefixes      存放目录前缀的vector
/// @retval true          提取目录前缀成功
/// @retval false         （反转）URL无效，提取目录前缀失败
bool UrlUtil::GetPrefixesOfReversedUrl(const std::string & reversed_url,
    std::vector<std::string> & prefixes)
{
    Url url_obj;

    if (url_obj.LoadReversedUrl(reversed_url))
    {
        return UrlUtil::GetPrefixesOfUrl(url_obj, prefixes);
    }

    return false;
}

/// 获取（反转）URL的所有（反转）目录前缀
/// @param  reversed_url      （反转）URL的字符串
/// @param  reversed_prefixes 存放（反转）目录前缀的vector
/// @retval true              提取（反转）目录前缀成功
/// @retval false             （反转）URL无效，提取（反转）目录前缀失败
bool UrlUtil::GetReversedPrefixesOfReversedUrl(const std::string & reversed_url,
    std::vector<std::string> & reversed_prefixes)
{
    Url url_obj;

    if (url_obj.LoadReversedUrl(reversed_url))
    {
        return UrlUtil::GetReversedPrefixesOfUrl(url_obj, reversed_prefixes);
    }

    return false;
}

} // namespace url
} // namespace web
