// Copyright (c) 2011, Tencent Inc. All rights reserved.
// Author:  Wen Jie <welkinwen@tencent.com>
// Created: 2011-04-20

#include "common/web/uri.h"
#include <stdio.h>
#include <stdlib.h>

namespace web {
namespace url {

#ifdef _WIN32
#ifndef STRNCASECMP
#define STRNCASECMP _strnicmp
#endif
#ifndef STRCASECMP
#define STRCASECMP  _stricmp
#endif
#ifndef snprintf
#define snprintf    _snprintf
#endif
#else // _WIN32
#include <strings.h>
#ifndef STRNCASECMP
#define STRNCASECMP strncasecmp
#endif
#ifndef STRCASECMP
#define STRCASECMP  strcasecmp
#endif
#endif // _WIN32

/// 协议信息
struct Scheme
{
    /// 协议类型枚举
    enum
    {
        kSchemeHttp,
        kSchemeHttps,
        kSchemeFtp
    };

    std::string m_scheme_string;    ///< 协议名称的字符串
    int m_default_port;             ///< 协议的默认端口
};

const Scheme SCHEMES[] =
{
    { "http", 80 },
    { "https", 443 },
    { "ftp", 21 }
};


Uri::Uri()
{
    Clear();
}

Uri::Uri(const std::string & uri_string)
{
    Load(uri_string);
}

Uri::~Uri()
{
}

bool Uri::Load(const std::string & uri_string)
{
    /// 初始化清空对象
    Clear();

    /// 解析组件
    if (uri_string.empty() || !Parse(uri_string))
    {
        return false;
    }

    /// 规范化
    Normalize();

    /// 重组组件
    Recompose();

    return IsValid();
}

bool Uri::Load(const std::string & uri_string, const std::string & base_uri_string)
{
    /// 初始化清空对象
    Clear();

    /// 解析组件
    if (uri_string.empty() || !Parse(uri_string))
    {
        return false;
    }

    /// 合并base_uri
    if (!IsAbsolute())
    {
        Uri base_uri;
        if (base_uri.Parse(base_uri_string))
        {
            Merge(base_uri);
        }
    }

    /// 规范化
    Normalize();

    /// 重组组件
    Recompose();

    return IsValid();
}

bool Uri::Merge(Uri & base_uri)
{
    if (!(m_defined_flag & kDefinedScheme) && (base_uri.m_defined_flag & kDefinedScheme))
    {
        m_scheme = base_uri.m_scheme;
        m_defined_flag |= kDefinedScheme;
    }

    if (!(m_defined_flag & kDefinedAuthority) && (base_uri.m_defined_flag & kDefinedAuthority))
    {
        m_authority = base_uri.m_authority;
        m_defined_flag |= kDefinedAuthority;

        if (!ParseAuthority())
        {
            m_parse_succ_flag = false;
        }
    }

    if (!m_path.empty() && ('/' != m_path[0]))
    {
        std::string::size_type pos = base_uri.m_path.rfind('/');
        if (std::string::npos != pos)
        {
            m_path = base_uri.m_path.substr(0, pos + 1) + m_path;
        }
    }

    return true;
}

bool Uri::IsAbsolute()
{
    return (m_defined_flag & kDefinedScheme);
}

bool Uri::IsValid()
{
    return false;
}

void Uri::Clear()
{
    m_scheme.clear();
    m_authority.clear();
    m_user.clear();
    m_password.clear();
    m_host.clear();
    m_port = 0;
    m_path.clear();
    m_query.clear();
    m_fragment.clear();

    m_absolute_uri.clear();
    m_relative_uri.clear();

    m_defined_flag = 0;
    m_parse_succ_flag = false;
    m_display_port_flag = false;
}

bool Uri::Parse(const std::string & uri_string)
{
    /// 划分组件
    SeparateComponents(uri_string);

    /// 解析组件
    m_parse_succ_flag = true;
    m_parse_succ_flag = m_parse_succ_flag && ParseScheme();
    m_parse_succ_flag = m_parse_succ_flag && ParseAuthority();
    m_parse_succ_flag = m_parse_succ_flag && ParsePath();
    m_parse_succ_flag = m_parse_succ_flag && ParseQuery();
    m_parse_succ_flag = m_parse_succ_flag && ParseFragment();

    return m_parse_succ_flag;
}

void Uri::Normalize()
{
    /// 如果定义了域名且路径非空，则路径必须以斜杠开头
    if ((m_defined_flag & kDefinedAuthority) && !m_path.empty() && ('/' != m_path[0]))
    {
        m_path = "/" + m_path;
    }

    RemoveDotSegments(m_path);
}

void Uri::Recompose()
{
    m_absolute_uri.clear();
    m_relative_uri.clear();

    if (m_defined_flag & kDefinedScheme)
    {
        m_absolute_uri.append(m_scheme);
        m_absolute_uri.append(1, ':');
    }

    if (m_defined_flag & kDefinedAuthority)
    {
        m_absolute_uri.append("//");
        m_authority.clear();

        if (m_defined_flag & kDefinedUser)
        {
            m_absolute_uri.append(m_user);
            m_authority.append(m_user);
            if (m_defined_flag & kDefinedPassword)
            {
                m_absolute_uri.append(1, ':');
                m_absolute_uri.append(m_password);

                m_authority.append(1, ':');
                m_authority.append(m_password);
            }
            m_absolute_uri.append(1, '@');
            m_authority.append(1, '@');
        }

        if (m_defined_flag & kDefinedHost)
        {
            m_absolute_uri.append(m_host);
            m_authority.append(m_host);
        }

        if ((m_defined_flag & kDefinedPort) && m_display_port_flag)
        {
            m_absolute_uri.append(1, ':');
            m_authority.append(1, ':');
            char port[8];
            snprintf(port, sizeof(port), "%d", m_port);
            m_absolute_uri.append(port);
            m_authority.append(port);
        }
    }

    m_absolute_uri.append(m_path);
    m_relative_uri.append(m_path);

    if (m_defined_flag & kDefinedQuery)
    {
        m_absolute_uri.append(1, '?');
        m_absolute_uri.append(m_query);

        m_relative_uri.append(1, '?');
        m_relative_uri.append(m_query);
    }

    if (m_defined_flag & kDefinedFragment)
    {
        m_absolute_uri.append(1, '#');
        m_absolute_uri.append(m_fragment);

        m_relative_uri.append(1, '#');
        m_relative_uri.append(m_fragment);
    }
}

void Uri::SeparateComponents(const std::string & uri_string)
{
    char ch;
    std::string::size_type start_pos, read_pos;

    start_pos = read_pos = 0;

    /// 协议名必须以字母开头
    if (isalpha((unsigned char)uri_string[read_pos]))
    {
        /// 尝试寻找协议名
        read_pos++;
        while (read_pos < uri_string.length())
        {
            ch = uri_string[read_pos];
            if (':' == ch)
            {
                m_scheme = uri_string.substr(start_pos, read_pos - start_pos);
                m_defined_flag |= kDefinedScheme;
                read_pos++;
                start_pos = read_pos;
                break;
            }
            else if (('/' == ch) || ('?' == ch) || ('#' == ch)
                     || (!isalpha((unsigned char)ch) && !isdigit((unsigned char)ch)
                         && ('+' != ch) && ('-' != ch) && ('.' != ch)))
            {
                break;
            }
            read_pos++;
        }
    }

    if (!(m_defined_flag & kDefinedScheme))
    {
        /// 未寻找到协议名，从头开始匹配其他组件
        read_pos = 0;
    }

    if ((read_pos + 1 < uri_string.length())
        && ('/' == uri_string[read_pos])
        && ('/' == uri_string[read_pos + 1]))
    {
        /// 匹配域名组件
        read_pos += 2;
        start_pos = read_pos;
        while (read_pos < uri_string.length())
        {
            ch = uri_string[read_pos];
            if (('/' == ch) || ('?' == ch) || ('#' == ch))
            {
                break;
            }
            read_pos++;
        }
        m_authority = uri_string.substr(start_pos, read_pos - start_pos);
        m_defined_flag |= kDefinedAuthority;
        start_pos = read_pos;
    }

    if (read_pos >= uri_string.length())
    {
        return;
    }

    /// 匹配路径组件
    while (read_pos < uri_string.length())
    {
        ch = uri_string[read_pos];
        if (('?' == ch) || ('#' == ch))
        {
            break;
        }
        read_pos++;
    }
    m_path = uri_string.substr(start_pos, read_pos - start_pos);
    m_defined_flag |= kDefinedPath;

    if (read_pos >= uri_string.length())
    {
        return;
    }

    if ('?' == uri_string[read_pos])
    {
        /// 匹配查询组件
        read_pos++;
        start_pos = read_pos;
        while ((read_pos < uri_string.length()) && ('#' != uri_string[read_pos]))
        {
            read_pos++;
        }
        m_query = uri_string.substr(start_pos, read_pos - start_pos);
        m_defined_flag |= kDefinedQuery;
    }

    if (read_pos < uri_string.length())
    {
        /// 匹配片段组件
        m_fragment = uri_string.substr(read_pos + 1, uri_string.length() - read_pos - 1);
        m_defined_flag |= kDefinedFragment;
    }
}

bool Uri::ParseScheme()
{
    if (m_defined_flag & kDefinedScheme)
    {
        /// 协议名全转小写
        for (std::string::size_type pos = 0; pos < m_scheme.length(); ++pos)
        {
            m_scheme[pos] = tolower(m_scheme[pos]);
        }
    }

    return true;
}

bool Uri::ParseAuthority()
{
    if (!(m_defined_flag & kDefinedAuthority) || (m_authority.empty()))
    {
        return true;
    }

    std::string::size_type start_pos, separate_pos;

    /// 寻找用户信息
    separate_pos = m_authority.find('@');
    if (std::string::npos != separate_pos)
    {
        std::string userinfo = m_authority.substr(0, separate_pos);
        std::string::size_type password_pos = userinfo.find(':');
        if (std::string::npos != password_pos)
        {
            m_user = userinfo.substr(0, password_pos);
            m_defined_flag |= kDefinedUser;
            m_password = userinfo.substr(password_pos + 1);
            m_defined_flag |= kDefinedPassword;
        }
        else
        {
            m_user = userinfo;
            m_defined_flag |= kDefinedUser;
        }

        start_pos = separate_pos + 1;
    }
    else
    {
        start_pos = 0;
    }

    /// 寻找端口信息
    separate_pos = m_authority.find(':', start_pos);
    std::string port;
    if (std::string::npos != separate_pos)
    {
        m_host = m_authority.substr(start_pos, separate_pos - start_pos);
        m_defined_flag |= kDefinedHost;
        port = m_authority.substr(separate_pos + 1);
        m_defined_flag |= kDefinedPort;
    }
    else
    {
        m_host = m_authority.substr(start_pos);
        m_defined_flag |= kDefinedHost;
    }
    AssignPort(port);

    /// 域名全转小写
    for (std::string::size_type pos = 0; pos < m_host.length(); ++pos)
    {
        m_host[pos] = tolower(m_host[pos]);
    }

    return true;
}

bool Uri::ParsePath()
{
    return true;
}

bool Uri::ParseQuery()
{
    return true;
}

bool Uri::ParseFragment()
{
    return true;
}

void Uri::AssignPort(const std::string & port)
{
    if (port.empty())
    {
        m_display_port_flag = false;
    }
    else
    {
        m_port = atoi(port.c_str());
        m_display_port_flag = true;
    }
}

void Uri::RemoveDotSegments(std::string & path)
{
    std::string::size_type read_pos, write_pos;
    read_pos = write_pos = 0;

    while (read_pos < path.length())
    {
        if (('.' == path[read_pos])
            && ((0 == write_pos) || ('/' == path[write_pos - 1])))
        {
            if (read_pos + 1 < path.length())
            {
                if ('.' == path[read_pos + 1])
                {
                    if (read_pos + 2 < path.length())
                    {
                        if ('/' == path[read_pos + 2])
                        {
                            /// 匹配<segment>/../
                            if (write_pos > 0)
                            {
                                write_pos--;
                            }

                            while ((write_pos >= 1) && ('/' != path[write_pos - 1]))
                            {
                                write_pos--;
                            }

                            read_pos += 3;

                            if (write_pos <= 0)
                            {
                                write_pos = 0;
                                path[write_pos++] = '/';
                            }
                            continue;
                        }
                    }
                    else
                    {
                        /// 匹配<<segment>/..
                        if (write_pos > 0)
                        {
                            write_pos--;
                        }
                        while ((write_pos >= 1) && ('/' != path[write_pos - 1]))
                        {
                            write_pos--;
                        }
                        break;
                    }
                }
                else if ('/' == path[read_pos + 1])
                {
                    /// 匹配<<segment>/./
                    read_pos += 2;
                    continue;
                }
            }
            else
            {
                /// 匹配<<segment>/.
                break;
            }
        }
        path[write_pos++] = path[read_pos++];
    }
    path.resize(write_pos);
}


HttpUri::HttpUri()
{
}

HttpUri::HttpUri(const std::string & uri_string)
{
    Load(uri_string);
}

HttpUri::~HttpUri()
{
}

bool HttpUri::IsAbsolute()
{
    return ((m_defined_flag & kDefinedScheme) && (m_defined_flag & kDefinedAuthority));
}

bool HttpUri::IsValid()
{
    return (m_parse_succ_flag && IsAbsolute() && !m_authority.empty());
}

void HttpUri::Normalize()
{
    /// 如果定义了域名且路径为空，则补全路径
    if ((m_defined_flag & kDefinedAuthority) && m_path.empty())
    {
        m_path = "/";
    }

    Uri::Normalize();
}

bool HttpUri::ParseScheme()
{
    Uri::ParseScheme();

    return (!(m_defined_flag & kDefinedScheme)
            || (SCHEMES[Scheme::kSchemeHttp].m_scheme_string == m_scheme));
}

void HttpUri::AssignPort(const std::string & port)
{
    if (port.empty())
    {
        m_port = SCHEMES[Scheme::kSchemeHttp].m_default_port;
        m_display_port_flag = false;
    }
    else
    {
        m_port = atoi(port.c_str());
        m_display_port_flag = (m_port != SCHEMES[Scheme::kSchemeHttp].m_default_port);
    }
}

HttpsUri::HttpsUri()
{
}

HttpsUri::HttpsUri(const std::string & uri_string)
{
    Load(uri_string);
}

HttpsUri::~HttpsUri()
{
}

bool HttpsUri::IsAbsolute()
{
    return ((m_defined_flag & kDefinedScheme) && (m_defined_flag & kDefinedAuthority));
}

bool HttpsUri::IsValid()
{
    return (m_parse_succ_flag && IsAbsolute() && !m_authority.empty());
}

void HttpsUri::Normalize()
{
    /// 如果定义了域名且路径为空，则补全路径
    if ((m_defined_flag & kDefinedAuthority) && m_path.empty())
    {
        m_path = "/";
    }

    Uri::Normalize();
}

bool HttpsUri::ParseScheme()
{
    Uri::ParseScheme();

    return (!(m_defined_flag & kDefinedScheme)
            || (SCHEMES[Scheme::kSchemeHttps].m_scheme_string == m_scheme));
}

void HttpsUri::AssignPort(const std::string & port)
{
    if (port.empty())
    {
        m_port = SCHEMES[Scheme::kSchemeHttps].m_default_port;
        m_display_port_flag = false;
    }
    else
    {
        m_port = atoi(port.c_str());
        m_display_port_flag = (m_port != SCHEMES[Scheme::kSchemeHttps].m_default_port);
    }
}

FtpUri::FtpUri()
{
}

FtpUri::FtpUri(const std::string & uri_string)
{
    Load(uri_string);
}

FtpUri::~FtpUri()
{
}

bool FtpUri::IsAbsolute()
{
    return ((m_defined_flag & kDefinedScheme) && (m_defined_flag & kDefinedAuthority));
}

bool FtpUri::IsValid()
{
    return (m_parse_succ_flag && IsAbsolute() && !m_authority.empty());
}

void FtpUri::Normalize()
{
    /// 如果定义了域名且路径为空，则补全路径
    if ((m_defined_flag & kDefinedAuthority) && m_path.empty())
    {
        m_path = "/";
    }

    Uri::Normalize();
}

bool FtpUri::ParseScheme()
{
    Uri::ParseScheme();

    return (!(m_defined_flag & kDefinedScheme)
            || (SCHEMES[Scheme::kSchemeFtp].m_scheme_string == m_scheme));
}

void FtpUri::AssignPort(const std::string & port)
{
    if (port.empty())
    {
        m_port = SCHEMES[Scheme::kSchemeFtp].m_default_port;
        m_display_port_flag = false;
    }
    else
    {
        m_port = atoi(port.c_str());
        m_display_port_flag = (m_port != SCHEMES[Scheme::kSchemeFtp].m_default_port);
    }
}

Uri * UriFactory::GetUri(const std::string & uri_string)
{
    if (uri_string.empty())
    {
        return NULL;
    }

    if ((('h' == uri_string[0]) || ('H' == uri_string[0]))
        && (0 == STRNCASECMP(uri_string.c_str() + 1, "ttp", 3)))
    {
        if ((uri_string.length() >= 5) && (':' == uri_string[4]))
        {
            return new HttpUri(uri_string);
        }
        else if ((uri_string.length() >= 6)
                 && (('s' == uri_string[4]) || ('S' == uri_string[4]))
                 && (':' == uri_string[5]))
        {
            return new HttpsUri(uri_string);
        }
    }
    else if ((('f' == uri_string[0]) || ('F' == uri_string[0]))
             && (0 == STRNCASECMP(uri_string.c_str() + 1, "tp:", 3)))
    {
        return new FtpUri(uri_string);
    }

    return new Uri(uri_string);
}

} // namespace url
} // namespace web
