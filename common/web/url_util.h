// Copyright (c) 2011, Tencent Inc. All rights reserved.
// Author:  Wen Jie <welkinwen@tencent.com>
// Created: 2011-04-20

#ifndef COMMON_WEB_URL_UTIL_H
#define COMMON_WEB_URL_UTIL_H

#include <string>
#include <vector>
#include "common/web/url.h"

namespace web {
namespace url {

/// URL工具类
class UrlUtil
{
public:
    /// 获取反转形式的片段路径的父目录
    /// @param  segment_path 反转形式的片段路径
    /// @return 反转形式的片段路径的父目录（同为反转形式）
    static std::string GetParentOfReversedSegmentPath(const std::string & segment_path);

    /// 获取URL的所有目录前缀
    /// @param  url      URL字符串
    /// @param  prefixes 存放目录前缀的vector
    /// @retval true     提取目录前缀成功
    /// @retval false    URL无效，提取目录前缀失败
    static bool GetPrefixesOfUrl(const std::string & url, std::vector<std::string> & prefixes);


    /// 获取URL的所有目录前缀
    /// @param  url      Url对象
    /// @param  prefixes 存放目录前缀的vector
    /// @retval true     提取目录前缀成功
    /// @retval false    Url对象无效，提取目录前缀失败
    static bool GetPrefixesOfUrl(const Url & url, std::vector<std::string> & prefixes);

    /// 获取URL的所有（反转）目录前缀
    /// @param  url               URL字符串
    /// @param  reversed_prefixes 存放（反转）目录前缀的vector
    /// @retval true              提取（反转）目录前缀成功
    /// @retval false             URL无效，提取（反转）目录前缀失败
    static bool GetReversedPrefixesOfUrl(const std::string & url,
        std::vector<std::string> & reversed_prefixes);

    /// 获取URL的所有（反转）目录前缀
    /// @param  url               Url对象
    /// @param  reversed_prefixes 存放（反转）目录前缀的vector
    /// @retval true              提取（反转）目录前缀成功
    /// @retval false             Url对象无效，提取（反转）目录前缀失败
    static bool GetReversedPrefixesOfUrl(const Url & url,
        std::vector<std::string> & reversed_prefixes);

    /// 获取（反转）URL的所有目录前缀
    /// @param  reversed_url  （反转）URL的字符串
    /// @param  prefixes      存放目录前缀的vector
    /// @retval true          提取目录前缀成功
    /// @retval false         （反转）URL无效，提取目录前缀失败
    static bool GetPrefixesOfReversedUrl(const std::string & reversed_url,
        std::vector<std::string> & prefixes);

    /// 获取（反转）URL的所有（反转）目录前缀
    /// @param  reversed_url      （反转）URL的字符串
    /// @param  reversed_prefixes 存放（反转）目录前缀的vector
    /// @retval true              提取（反转）目录前缀成功
    /// @retval false             （反转）URL无效，提取（反转）目录前缀失败
    static bool GetReversedPrefixesOfReversedUrl(const std::string & reversed_url,
        std::vector<std::string> & reversed_prefixes);
};

} // namespace url
} // namespace web

#endif // COMMON_WEB_URL_UTIL_H
