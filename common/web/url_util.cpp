// Copyright (c) 2011, Tencent Inc. All rights reserved.
// Author:  Wen Jie <welkinwen@tencent.com>
// Created: 2011-04-20

#include "common/web/url_util.h"
#include <stdio.h>

namespace web {
namespace url {

/// ��ȡ��ת��ʽ��Ƭ��·���ĸ�Ŀ¼
/// @param  segment_path ��ת��ʽ��Ƭ��·��
/// @return ��ת��ʽ��Ƭ��·���ĸ�Ŀ¼��ͬΪ��ת��ʽ��
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

/// ��ȡURL������Ŀ¼ǰ׺
/// @param  url      URL�ַ���
/// @param  prefixes ���Ŀ¼ǰ׺��vector
/// @retval true     ��ȡĿ¼ǰ׺�ɹ�
/// @retval false    URL��Ч����ȡĿ¼ǰ׺ʧ��
bool UrlUtil::GetPrefixesOfUrl(const std::string & url, std::vector<std::string> & prefixes)
{
    Url url_obj;

    if (url_obj.Load(url.c_str(), url.length()))
    {
        return UrlUtil::GetPrefixesOfUrl(url_obj, prefixes);
    }

    return false;
}

/// ��ȡURL������Ŀ¼ǰ׺
/// @param  url      Url����
/// @param  prefixes ���Ŀ¼ǰ׺��vector
/// @retval true     ��ȡĿ¼ǰ׺�ɹ�
/// @retval false    Url������Ч����ȡĿ¼ǰ׺ʧ��
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

/// ��ȡURL�����У���ת��Ŀ¼ǰ׺
/// @param  url               URL�ַ���
/// @param  reversed_prefixes ��ţ���ת��Ŀ¼ǰ׺��vector
/// @retval true              ��ȡ����ת��Ŀ¼ǰ׺�ɹ�
/// @retval false             URL��Ч����ȡ����ת��Ŀ¼ǰ׺ʧ��
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

/// ��ȡURL�����У���ת��Ŀ¼ǰ׺
/// @param  url               Url����
/// @param  reversed_prefixes ��ţ���ת��Ŀ¼ǰ׺��vector
/// @retval true              ��ȡ����ת��Ŀ¼ǰ׺�ɹ�
/// @retval false             Url������Ч����ȡ����ת��Ŀ¼ǰ׺ʧ��
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

/// ��ȡ����ת��URL������Ŀ¼ǰ׺
/// @param  reversed_url  ����ת��URL���ַ���
/// @param  prefixes      ���Ŀ¼ǰ׺��vector
/// @retval true          ��ȡĿ¼ǰ׺�ɹ�
/// @retval false         ����ת��URL��Ч����ȡĿ¼ǰ׺ʧ��
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

/// ��ȡ����ת��URL�����У���ת��Ŀ¼ǰ׺
/// @param  reversed_url      ����ת��URL���ַ���
/// @param  reversed_prefixes ��ţ���ת��Ŀ¼ǰ׺��vector
/// @retval true              ��ȡ����ת��Ŀ¼ǰ׺�ɹ�
/// @retval false             ����ת��URL��Ч����ȡ����ת��Ŀ¼ǰ׺ʧ��
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
