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

/// URL������
class UrlUtil
{
public:
    /// ��ȡ��ת��ʽ��Ƭ��·���ĸ�Ŀ¼
    /// @param  segment_path ��ת��ʽ��Ƭ��·��
    /// @return ��ת��ʽ��Ƭ��·���ĸ�Ŀ¼��ͬΪ��ת��ʽ��
    static std::string GetParentOfReversedSegmentPath(const std::string & segment_path);

    /// ��ȡURL������Ŀ¼ǰ׺
    /// @param  url      URL�ַ���
    /// @param  prefixes ���Ŀ¼ǰ׺��vector
    /// @retval true     ��ȡĿ¼ǰ׺�ɹ�
    /// @retval false    URL��Ч����ȡĿ¼ǰ׺ʧ��
    static bool GetPrefixesOfUrl(const std::string & url, std::vector<std::string> & prefixes);


    /// ��ȡURL������Ŀ¼ǰ׺
    /// @param  url      Url����
    /// @param  prefixes ���Ŀ¼ǰ׺��vector
    /// @retval true     ��ȡĿ¼ǰ׺�ɹ�
    /// @retval false    Url������Ч����ȡĿ¼ǰ׺ʧ��
    static bool GetPrefixesOfUrl(const Url & url, std::vector<std::string> & prefixes);

    /// ��ȡURL�����У���ת��Ŀ¼ǰ׺
    /// @param  url               URL�ַ���
    /// @param  reversed_prefixes ��ţ���ת��Ŀ¼ǰ׺��vector
    /// @retval true              ��ȡ����ת��Ŀ¼ǰ׺�ɹ�
    /// @retval false             URL��Ч����ȡ����ת��Ŀ¼ǰ׺ʧ��
    static bool GetReversedPrefixesOfUrl(const std::string & url,
        std::vector<std::string> & reversed_prefixes);

    /// ��ȡURL�����У���ת��Ŀ¼ǰ׺
    /// @param  url               Url����
    /// @param  reversed_prefixes ��ţ���ת��Ŀ¼ǰ׺��vector
    /// @retval true              ��ȡ����ת��Ŀ¼ǰ׺�ɹ�
    /// @retval false             Url������Ч����ȡ����ת��Ŀ¼ǰ׺ʧ��
    static bool GetReversedPrefixesOfUrl(const Url & url,
        std::vector<std::string> & reversed_prefixes);

    /// ��ȡ����ת��URL������Ŀ¼ǰ׺
    /// @param  reversed_url  ����ת��URL���ַ���
    /// @param  prefixes      ���Ŀ¼ǰ׺��vector
    /// @retval true          ��ȡĿ¼ǰ׺�ɹ�
    /// @retval false         ����ת��URL��Ч����ȡĿ¼ǰ׺ʧ��
    static bool GetPrefixesOfReversedUrl(const std::string & reversed_url,
        std::vector<std::string> & prefixes);

    /// ��ȡ����ת��URL�����У���ת��Ŀ¼ǰ׺
    /// @param  reversed_url      ����ת��URL���ַ���
    /// @param  reversed_prefixes ��ţ���ת��Ŀ¼ǰ׺��vector
    /// @retval true              ��ȡ����ת��Ŀ¼ǰ׺�ɹ�
    /// @retval false             ����ת��URL��Ч����ȡ����ת��Ŀ¼ǰ׺ʧ��
    static bool GetReversedPrefixesOfReversedUrl(const std::string & reversed_url,
        std::vector<std::string> & reversed_prefixes);
};

} // namespace url
} // namespace web

#endif // COMMON_WEB_URL_UTIL_H
