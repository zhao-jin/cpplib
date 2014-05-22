// Copyright (c) 2011, Tencent Inc. All rights reserved.
// Author:  Wen Jie <welkinwen@tencent.com>
// Created: 2011-04-20

#include "common/web/url.h"
#include <stdio.h>
#include <stdlib.h>
#include "common/base/string/byte_set.h"
#include "common/base/string/string_number.h"
#include "common/crypto/hash/md5.h"

namespace web {
namespace url {

/////////////////////////////////////////////////////
//                Consts Defination                //
/////////////////////////////////////////////////////

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
#ifndef STRNCASECMP
#define STRNCASECMP strncasecmp
#endif
#ifndef STRCASECMP
#define STRCASECMP  strcasecmp
#endif
#endif // _WIN32

#define URL_CHR_TEST(c, mask)   (URL_CHR_TABLE[(unsigned char)(c)] & (mask))
#define URL_RESERVED_CHAR(c)    URL_CHR_TEST(c, E_URL_CHR_RESERVED)
#define URL_UNSAFE_CHAR(c)      URL_CHR_TEST(c, E_URL_UNSAFE)

#define XNUM_TO_DIGIT(x)        ("0123456789ABCDEF"[x] + 0)
#define XDIGIT_TO_NUM(h)        ((h) < 'A' ? (h) - '0' : toupper(h) - 'A' + 10)
#define X2DIGITS_TO_NUM(h1, h2) ((XDIGIT_TO_NUM (h1) << 4) + XDIGIT_TO_NUM (h2))

#define IS_PERCENT_ENCODED_SPACE(h1, h2) \
    (isspace((unsigned char)(X2DIGITS_TO_NUM(h1, h2))))

/// Url的字符类别
enum
{
    E_URL_CHR_RESERVED = 1,
    E_URL_UNSAFE = 2
};

/// Shorthands for the table
#define R   E_URL_CHR_RESERVED
#define U   E_URL_UNSAFE
#define RU  R|U

/// Characters defined by RFC 3986
const unsigned char URL_CHR_TABLE[256] =
{
    U,  U,  U,  U,   U,  U,  U,  U,   /* NUL SOH STX ETX  EOT ENQ ACK BEL */
    U,  U,  U,  U,   U,  U,  U,  U,   /* BS  HT  LF  VT   FF  CR  SO  SI  */
    U,  U,  U,  U,   U,  U,  U,  U,   /* DLE DC1 DC2 DC3  DC4 NAK SYN ETB */
    U,  U,  U,  U,   U,  U,  U,  U,   /* CAN EM  SUB ESC  FS  GS  RS  US  */
    U,  R,  U, RU,   R,  U,  R,  R,   /* SP  !   "   #    $   %   &   '   */
    R,  R,  R,  R,   R,  0,  0,  R,   /* (   )   *   +    ,   -   .   /   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* 0   1   2   3    4   5   6   7   */
    0,  0, RU,  R,   U,  R,  U,  R,   /* 8   9   :   ;    <   =   >   ?   */
    RU, 0,  0,  0,   0,  0,  0,  0,   /* @   A   B   C    D   E   F   G   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* H   I   J   K    L   M   N   O   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* P   Q   R   S    T   U   V   W   */
    0,  0,  0, RU,   U, RU,  U,  0,   /* X   Y   Z   [    \   ]   ^   _   */
    U,  0,  0,  0,   0,  0,  0,  0,   /* `   a   b   c    d   e   f   g   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* h   i   j   k    l   m   n   o   */
    0,  0,  0,  0,   0,  0,  0,  0,   /* p   q   r   s    t   u   v   w   */
    0,  0,  0,  U,   U,  U,  0,  U,   /* x   y   z   {    |   }   ~   DEL */

    U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
    U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
    U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
    U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,

    U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
    U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
    U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
    U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
};

#undef RU
#undef U
#undef R

// const ByteSet GEN_DELIMS_SET(":/?#[]@");
// const ByteSet SUB_DELIMS_SET("!$&'()*+,;=");
// const ByteSet RESERVED_SET(":/?#[]@!$&'()*+,;=");
// const ByteSet UNRESERVED_SET("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
//     "-._~");
// const ByteSet PCHAR_SET("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
//     "-._~!$&'()*+,;=:@");
const ByteSet SCHEME_SET("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
    "+-.");
const ByteSet DOMAIN_SET("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
    "-._");
// const ByteSet PATH_SET("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
//     "-._~!$&'()*+,;=:@/");
// const ByteSet QUERY_SET("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
//     "-._~!$&'()*+,;=:@/?");

const int MAX_SCHEME_LENGTH = 8;
const int MAX_PORT_LENGTH = 5;

/// 协议信息
struct SSchemeInfo
{
    std::string m_scheme_string;    ///< 协议名称的字符串
    uint16_t m_default_port;        ///< 协议的默认端口
};

/// 协议定义，新增协议在此添加
const SSchemeInfo SCHEMES[] =
{
    { "http", 80 },             ///< E_HTTP_PROTOCOL
    { "https", 443 },           ///< E_HTTPS_PROTOCOL
    { "ftp", 21 },              ///< E_FTP_PROTOCOL
};

////////////////////////////////////////////////////////
//                Class Implementation                //
////////////////////////////////////////////////////////

////////////////// Url //////////////////

Url::Url()
{
    Clear();
}

Url::Url(const char * url, const int url_len,
         EScheme default_scheme,
         int normalize_flag, int valid_schemes)
{
    Load(url, url_len, default_scheme, normalize_flag, valid_schemes);
}

Url::Url(const char * url, const int url_len,
         const char * base_url, const int base_url_len,
         int normalize_flag, int valid_schemes)
{
    Load(url, url_len, base_url, base_url_len, normalize_flag, valid_schemes);
}

Url::Url(const char * url, const int url_len,
         const Url & base_url,
         int normalize_flag, int valid_schemes)
{
    Load(url, url_len, base_url, normalize_flag, valid_schemes);
}

bool Url::Load(const char * url, const int url_len,
               EScheme default_scheme,
               int normalize_flag, int valid_schemes)
{
    Clear();

    if ((url == NULL) || (url_len <= 0))
    {
        return false;
    }

    m_valid_schemes = valid_schemes;

    if (normalize_flag & E_NORMALIZE_PREPROCESS)
    {
        std::string preprocessed_url;

        if (!Preprocess(url, url_len,
                        &preprocessed_url,
                        normalize_flag & E_NORMALIZE_DECODE_HEXADECIMAL)
            || !Parse(preprocessed_url.c_str(), preprocessed_url.length(), default_scheme))
        {
            return false;
        }
    }
    else
    {
        if (!Parse(url, url_len, default_scheme))
        {
            return false;
        }
    }

    Recompose(normalize_flag);

    return IsValid();
}

bool Url::Load(const char * url, const int url_len,
               const char * base_url, const int base_url_len,
               int normalize_flag, int valid_schemes)
{
    Url base_url_obj(base_url, base_url_len, E_NO_PROTOCOL, normalize_flag, valid_schemes);
    return Load(url, url_len, base_url_obj, normalize_flag, valid_schemes);
}

bool Url::Load(const char * url, const int url_len,
               const Url & base_url,
               int normalize_flag, int valid_schemes)
{
    Clear();

    /***********************************************************
    * Step 1:
    * The base URL is established according to the rules of
    * Section 3. If the base URL is the empty string (unknown),
    * the embedded URL is interpreted as an absolute URL and
    * we are done.
    ***********************************************************/

    /***********************************************************
    * Step 2:
    * Both the base and embedded URLs are parsed into their
    * component parts as described in RFC1808 Section 2.4.

    * a)
    * If the embedded URL is entirely empty, it inherits the
    * entire base URL (i.e., is set equal to the base URL)
    * and we are done.
    ***********************************************************/
    if ((url == NULL) || (url_len <= 0))
    {
        return false;
    }

    m_valid_schemes = valid_schemes;

    /***********************************************************
    * b)
    * If the embedded URL starts with a scheme name, it is
    * interpreted as an absolute URL and we are done.
    ***********************************************************/
    if (normalize_flag & E_NORMALIZE_PREPROCESS)
    {
        std::string preprocessed_url;

        if (!Preprocess(url, url_len,
                        &preprocessed_url,
                        normalize_flag & E_NORMALIZE_DECODE_HEXADECIMAL)
            || !Parse(preprocessed_url.c_str(), preprocessed_url.length(), E_NO_PROTOCOL))
        {
            return false;
        }
    }
    else
    {
        if (!Parse(url, url_len, E_NO_PROTOCOL))
        {
            return false;
        }
    }

    if (m_scheme != E_NO_PROTOCOL)
    {
        Recompose(normalize_flag);
        return IsValid();   // Step 7
    }

    /**********************************************************
    * c)
    * Otherwise, the embedded URL inherits the scheme of
    * the base URL.
    ***********************************************************/
    if (!base_url.IsValid()) return false;

    m_scheme = base_url.m_scheme;

    /***********************************************************
    * Step 3:
    * If the embedded URL's <net_loc> is non-empty, we skip to
    * Step 7. Otherwise, the embedded URL inherits the <net_loc>
    * (if any) of the base URL.
    ***********************************************************/
    if (!m_host.empty())
    {
        if (m_port == 0)
        {
            m_port = base_url.m_port;
        }

        Recompose(normalize_flag);
        return IsValid();   // Step 7
    }
    else
    {
        CopyHostInfo(base_url);
    }

    /***********************************************************
    * Step 4:
    * If the embedded URL path is preceded by a slash "/", the
    * path is not relative and we skip to Step 7.
    ***********************************************************/
    if (!m_path.empty() && (m_path[0] == '/'))
    {
        Recompose(normalize_flag);
        return IsValid();   // not relative and goto Step 7
    }

    /***********************************************************
    * Step 5:
    * If the embedded URL path is empty (and not preceded by a
    * slash), then the embedded URL inherits the base URL path,
    * and
    ***********************************************************/
    if (m_path.empty())
    {
        CopyPathInfo(base_url);

        /*********************************************************
        * a)
        * if the embedded URL's <params> is non-empty, we skip to
        * step 7; otherwise, it inherits the <params> of the base
        * URL (if any) and
        **********************************************************/

        /********************************************************
        * b)
        * if the embedded URL's <query> is non-empty, we skip to
        * step 7; otherwise, it inherits the <query> of the base
        * URL (if any) and we skip to step 7.
        ********************************************************/
        if (m_query.empty())
        {
            CopyQueryInfo(base_url);
        }

        Recompose(normalize_flag);
        return IsValid();   // Step 7
    }

    /***********************************************************
    * Step 6:
    * The last segment of the base URL's path (anything
    * following the rightmost slash "/", or the entire path if no
    * slash is present) is removed and the embedded URL's path is
    * appended in its place. The following operations are
    * then applied, in order, to the new path:
    ***********************************************************************/
    JoinPath(base_url.m_path);

    /***********************************************************************
    * Step 7:
    * The resulting URL components, including any inherited from
    * the base URL, are recombined to give the absolute form of
    * the embedded URL.

    * Parameters, regardless of their purpose, do not form a part of the
    * URL path and thus do not affect the resolving of relative paths. In
    * particular, the presence or absence of the ";type=d" parameter on an
    * ftp URL does not affect the interpretation of paths relative to that
    * URL. Fragment identifiers are only inherited from the base URL when
    * the entire embedded URL is empty.

    * The above algorithm is intended to provide an example by which the
    * output of implementations can be tested -- implementation of the
    * algorithm itself is not required. For example, some systems may find
    * it more efficient to implement Step 6 as a pair of segment stacks
    * being merged, rather than as a series of string pattern matches.
    ***********************************************************************/

    Recompose(normalize_flag);
    return IsValid();
}

bool Url::LoadReversedUrl(const void * reversed_url, const int reversed_url_len,
                          EScheme default_scheme, int normalize_flag, int valid_schemes)
{
    Clear();

    m_valid_schemes = valid_schemes;

    const char * pointer = reinterpret_cast<const char *>(reversed_url);
    int start_pos;
    int pos = 0;

    /// 域名
    while ((pos < reversed_url_len) && (pointer[pos] != '#') && (pointer[pos] != '"')) pos++;
    m_host = ReverseDomain(std::string(pointer, pos));

    if (pos >= reversed_url_len)
    {
        if (IsSchemeValid(default_scheme, m_valid_schemes))
        {
            SetSchemeAndPort(default_scheme);
            SetDefaultPath();

            Recompose(normalize_flag);
            return IsValid();
        }
        else
        {
            return false;
        }
    }

    /// 端口
    if (pointer[pos] == '#')
    {
        char port[MAX_PORT_LENGTH + 2];

        pos++;
        start_pos = pos;
        while ((pos < reversed_url_len) && (pointer[pos] != '"')
               && (pos - start_pos <= MAX_PORT_LENGTH))
        {
            port[pos - start_pos] = pointer[pos];
            pos++;
        }

        if (pos - start_pos > MAX_PORT_LENGTH)
        {
            return false;
        }
        else
        {
            port[pos - start_pos] = '\0';
        }

        char * end_ptr;
        if (!ParseNumber(port, &m_port, &end_ptr))
        {
            return false;
        }

        if (pos >= reversed_url_len)
        {
            if (IsSchemeValid(default_scheme, m_valid_schemes))
            {
                SetSchemeAndPort(default_scheme);
                SetDefaultPath();

                Recompose(normalize_flag);
                return IsValid();
            }
            else
            {
                return false;
            }
        }
    }

    /// 协议
    pos++;
    start_pos = pos;
    while ((pos < reversed_url_len) && (pointer[pos] != '"')) pos++;
    if (pos >= reversed_url_len) return false;
    int scheme_index;
    int scheme_count = sizeof(SCHEMES) / sizeof(SSchemeInfo);
    for (scheme_index = 0; scheme_index < scheme_count; scheme_index++)
    {
        if (STRNCASECMP(pointer + start_pos, SCHEMES[scheme_index].m_scheme_string.c_str(),
                        pos - start_pos) == 0)
        {
            SetSchemeAndPort(static_cast<EScheme>(scheme_index));
            break;
        }
    }
    if (scheme_index == scheme_count)
    {
        return false;
    }

    /// 路径
    pos++;
    start_pos = pos;
    while ((pos < reversed_url_len) && (pointer[pos] != '?')) pos++;
    m_path.assign(pointer + start_pos, pos - start_pos);
    if (m_path.empty())
    {
        SetDefaultPath();
    }

    /// 查询
    if (pos < reversed_url_len)
    {
        m_has_query = true;
        start_pos = pos + 1;
        m_query.assign(pointer + start_pos, reversed_url_len - start_pos);
        ParseQueryString(m_query, &m_query_pos_list);
    }

    Recompose(normalize_flag);
    return IsValid();
}

std::string Url::GetNormalizedUrl(int normalize_flag) const
{
    if (!IsValid())
    {
        return std::string("");
    }

    std::string normalized_url;
    normalized_url.reserve(m_absolute_path.length());

    /// 协议
    normalized_url = SCHEMES[m_scheme].m_scheme_string + "://";

    /// 域名
    normalized_url.append(m_host);

    /// 端口
    if (HasExplicitPort())
    {
        normalized_url.append(1, ':');
        normalized_url.append(NumberToString(m_port));
    }

    /// 路径
    if ((normalize_flag & E_NORMALIZE_TRUNCATE_DEFAULT_PATH)
        && !IsDynamicPage() && (m_directory_pos_list.size() == 1)
        && IsDefaultPage(GetResourcePtr()))
    {
        normalized_url.append(m_path.c_str(), m_resource_pos);
    }
    else
    {
        normalized_url.append(m_path);
    }

    /// 查询
    if (m_has_query)
    {
        std::string recomposed_query;

        if (normalize_flag & E_NORMALIZE_SORT_QUERY)
        {
            std::string query;
            std::vector<QueryPos> query_pos_list(m_query_pos_list);

            SortQuery(m_query, &query_pos_list);
            RecomposeQuery(m_query, query_pos_list, &query);

            if (normalize_flag & E_NORMALIZE_DECODE_HEXADECIMAL)
            {
                query = DecodeUrl(query);
            }

            if (normalize_flag & E_NORMALIZE_LOWER_CASE)
            {
                for (std::string::size_type i = 0; i < query.length(); ++i)
                {
                    query[i] = tolower(query[i]);
                }
            }

            /// Query may have been changed, parse and sort again
            query_pos_list.clear();
            ParseQueryString(query, &query_pos_list);
            SortQuery(query, &query_pos_list);
            RecomposeQuery(query, query_pos_list, &recomposed_query);
        }
        else
        {
            RecomposeQuery(m_query, m_query_pos_list, &recomposed_query);
        }

        normalized_url.append(1, '?');
        normalized_url.append(recomposed_query);
    }

    if ((normalize_flag & E_NORMALIZE_TRUNCATE_LAST_SLASH)
        && !IsDynamicPage() && (m_directory_pos_list.size() > 1)
        && (normalized_url[normalized_url.length() - 1] == '/'))
    {
        /// 静态目录页去除末尾的'/'
        normalized_url.erase(normalized_url.length() - 1);
    }

    if (normalize_flag & E_NORMALIZE_DECODE_HEXADECIMAL)
    {
        normalized_url = DecodeUrl(normalized_url);
    }

    if (normalize_flag & E_NORMALIZE_LOWER_CASE)
    {
        for (std::string::size_type i = 0; i < normalized_url.length(); ++i)
        {
            normalized_url[i] = tolower(normalized_url[i]);
        }
    }

    return normalized_url;
}

bool Url::GetReversedUrl(void * reversed_url, int & reversed_url_len,
                         int normalize_flag) const
{
    /// 考虑域名在域名表和URL表中的字典序：
    /// ------------------------------------------------------
    /// | Domain Table         | URL Table                   |
    /// ------------------------------------------------------
    /// | 1) com.sample.x      | 1) com.sample.x.xx<http>/   |
    /// | 2) com.sample.x.xx   | 2) com.sample.x:8080<http>/ |
    /// | 3) com.sample.x:8080 | 3) com.sample.x<http>/      |
    /// ------------------------------------------------------
    /// 为满足以下两个条件：
    /// 1) 保持两个表中的字典序一致
    /// 2) com.sample.x和com.sample.x:8080紧邻
    /// 则需要：'scheme的标识符' < 'port的标识符' < '.'
    /// 使用'"'作为scheme的标识符，'#'作为port的标识符，则新字典序为：
    /// ------------------------------------------------------
    /// | Domain Table         | URL Table                   |
    /// ------------------------------------------------------
    /// | 1) com.sample.x      | 1) com.sample.x"http"/      |
    /// | 2) com.sample.x#8080 | 2) com.sample.x#8080"http"/ |
    /// | 3) com.sample.x.xx   | 3) com.sample.x.xx"http"/   |
    /// ------------------------------------------------------

    if (!IsValid())
    {
        return false;
    }

    std::string normalized_url = GetNormalizedUrl(normalize_flag);
    std::string::size_type pos = normalized_url.find("://");
    if (pos == std::string::npos)
    {
        return false;
    }
    pos = normalized_url.find('/', pos + 3);
    if (pos == std::string::npos)
    {
        return false;
    }

    std::string reversed_host_port = GetReversedHostPort();

    char * pointer = static_cast<char *>(reversed_url);

#define APPEND_STRING(name, length) \
    memcpy(pointer, name, length); \
    pointer += length

    APPEND_STRING(reversed_host_port.c_str(), reversed_host_port.length());
    APPEND_STRING("\"", 1);
    APPEND_STRING(SCHEMES[m_scheme].m_scheme_string.c_str(),
                  SCHEMES[m_scheme].m_scheme_string.length());
    APPEND_STRING("\"", 1);
    APPEND_STRING(normalized_url.c_str() + pos, normalized_url.length() - pos);

#undef APPEND_STRING

    reversed_url_len = pointer - static_cast<char *>(reversed_url);

    return true;
}

std::string Url::GetReversedUrl(int normalize_flag) const
{
    if (!IsValid())
    {
        return std::string("");
    }

    std::string normalized_url = GetNormalizedUrl(normalize_flag);
    std::string::size_type pos = normalized_url.find("://");
    if (pos == std::string::npos)
    {
        return std::string("");
    }
    pos = normalized_url.find('/', pos + 3);
    if (pos == std::string::npos)
    {
        return std::string("");
    }

    std::string reversed_url = GetReversedHostPort()
                             + "\"" + SCHEMES[m_scheme].m_scheme_string + "\""
                             + normalized_url.substr(pos);

    return reversed_url;
}

std::string Url::GetSegmentPath() const
{
    if (!IsValid())
    {
        return std::string("");
    }

    if (IsDynamicPage())
    {
        if (m_portal_path_end_pos != std::string::npos)
        {
            return m_absolute_path.substr(0, m_portal_path_end_pos + 1);
        }
    }
    else
    {
        std::string::size_type last_slash_pos = m_absolute_path.rfind('/');
        if (last_slash_pos != std::string::npos)
        {
            return m_absolute_path.substr(0, last_slash_pos + 1);
        }
    }

    return std::string("");
}

std::string Url::GetReversedSegmentPath() const
{
    std::string reversed_url = GetReversedUrl(0);
    std::string::size_type pos;

    if (IsDynamicPage())
    {
        pos = reversed_url.find('?');
        if (pos != std::string::npos)
        {
            return reversed_url.substr(0, pos + 1);
        }
    }
    else
    {
        pos = reversed_url.rfind('/');
        if (pos != std::string::npos)
        {
            return reversed_url.substr(0, pos + 1);
        }
    }

    return std::string("");
}

std::string Url::GetSchemeString() const
{
    return IsSchemeValid(m_scheme, m_valid_schemes) ?
        SCHEMES[m_scheme].m_scheme_string : std::string("");
}

std::string Url::GetHostPort() const
{
    if (HasExplicitPort())
    {
        std::string host(m_host);
        host.append(1, ':');
        host.append(NumberToString(m_port));

        return host;
    }

    return m_host;
}

void Url::GetQuerys(std::map<std::string, std::string> & querys) const
{
    std::string key;
    std::string value;
    std::vector<QueryPos>::const_iterator it;

    for (it = m_query_pos_list.begin(); it != m_query_pos_list.end(); ++it)
    {
        key.assign(m_query, it->m_key_begin, it->m_key_end - it->m_key_begin);

        if ((it->m_value_begin != std::string::npos)
            && (it->m_value_end != std::string::npos))
        {
            value.assign(m_query, it->m_value_begin, it->m_value_end - it->m_value_begin);
        }
        else
        {
            value.clear();
        }

        querys[key] = value;
    }
}

std::string Url::GetDirectory() const
{
    if (!m_directory_pos_list.empty())
    {
        return m_path.substr(m_directory_pos_list[0],
            m_directory_pos_list[m_directory_pos_list.size() - 1] - m_directory_pos_list[0] + 1);
    }

    return std::string("");
}

void Url::GetDirectories(std::vector<std::string> & directories) const
{
    std::vector<std::string::size_type>::const_iterator it;
    for (it = m_directory_pos_list.begin(); it != m_directory_pos_list.end(); ++it)
    {
        directories.push_back(m_path.substr(0, *it + 1));
    }
}

std::string Url::GetResourceSuffix() const
{
    const char * suffix_ptr = GetResourceSuffixPtr();

    if (suffix_ptr != NULL)
    {
        return std::string(suffix_ptr);
    }

    return std::string("");
}

const char * Url::GetResourceSuffixPtr() const
{
    if (m_resource_pos != std::string::npos)
    {
        std::string::size_type pos = m_path.length() - 1;
        while (pos >= m_resource_pos)
        {
            if (m_path[pos] == '.')
            {
                if (pos == (m_path.length() - 1))
                {
                    /// exclude resource end with '.'
                    return NULL;
                }
                else
                {
                    return (m_path.c_str() + pos + 1);
                }
            }
            pos--;
        }
    }

    return NULL;
}

std::string Url::EncodeUrl(const std::string & url, int flag)
{
    std::string encoded_url;
    std::string::size_type url_len = url.length();
    std::string::size_type pos;
    unsigned char ch;

    for (pos = 0; pos < url_len; pos++)
    {
        ch = url.at(pos);

        if (ch == '%')
        {
            if (((pos + 2) < url_len)
                && isxdigit((unsigned char)url.at(pos + 1))
                && isxdigit((unsigned char)url.at(pos + 2)))
            {
                encoded_url += ch;
            }
            else
            {
                encoded_url += '%';
                encoded_url += XNUM_TO_DIGIT(ch >> 4);
                encoded_url += XNUM_TO_DIGIT(ch & 0xf);
            }
        }
        else if (URL_UNSAFE_CHAR (ch))
        {
            if ((!(flag & E_ENCODE_RESERVED_CHAR) && URL_RESERVED_CHAR (ch))
                || ((flag & E_NOT_ENCODE_EXTENDED_CHAR_SET) && (ch > 127)))
            {
                encoded_url += ch;
            }
            else
            {
                encoded_url += '%';
                encoded_url += XNUM_TO_DIGIT(ch >> 4);
                encoded_url += XNUM_TO_DIGIT(ch & 0xf);
            }
        }
        else
        {
            if ((flag & E_ENCODE_RESERVED_CHAR) && URL_RESERVED_CHAR(ch))
            {
                encoded_url += '%';
                encoded_url += XNUM_TO_DIGIT(ch >> 4);
                encoded_url += XNUM_TO_DIGIT(ch & 0xf);
            }
            else
            {
                encoded_url += ch;
            }
        }
    }

    return encoded_url;
}

std::string Url::DecodeUrl(const std::string & url, int flag)
{
    unsigned char ch;
    std::string::size_type pos;
    std::string decoded_url;
    const char * url_string = url.c_str();
    unsigned int url_len = url.length();

    for (pos = 0; pos < url_len; pos++)
    {
        if ((url_string[pos] == '%') && (pos + 2 < url_len)
            && isxdigit((unsigned char)(url_string[pos + 1]))
            && isxdigit((unsigned char)(url_string[pos + 2])))
        {
            ch = X2DIGITS_TO_NUM(url_string[pos + 1], url_string[pos + 2]);

            if ((!(flag & E_DECODE_RESERVED_CHAR) && URL_RESERVED_CHAR(ch))
                || (!(flag & E_DECODE_PERCENT_SIGN_CHAR) && (ch == '%')))
            {
                decoded_url.push_back(url_string[pos]);
                decoded_url.push_back(url_string[pos + 1]);
                decoded_url.push_back(url_string[pos + 2]);
            }
            else
            {
                decoded_url.push_back(ch);
            }

            pos += 2;
        }
        else
        {
            decoded_url.push_back(url_string[pos]);
        }
    }

    return decoded_url;
}

std::string Url::ReverseDomain(const std::string & domain)
{
    if (!domain.empty())
    {
        std::string reversed_domain;
        reversed_domain.resize(domain.length());

        if (ReverseDomain(domain.c_str(), domain.length(), &reversed_domain[0]) != NULL)
        {
            return reversed_domain;
        }
    }

    return std::string("");
}

char * Url::ReverseDomain(const char * src, const int src_len, char * dest)
{
    if ((src == NULL) || (dest == NULL))
    {
        return NULL;
    }

    int write_len;
    int write_pos = 0;
    int port_pos = -1;
    int end_pos = src_len;

    for (int pos = src_len - 1; pos > 0; pos--)
    {
        if (src[pos] == '.')
        {
            write_len = end_pos - pos - 1;
            memcpy(dest + write_pos, src + pos + 1, write_len);
            write_pos += write_len;
            dest[write_pos++] = '.';
            end_pos = pos;
        }
        else if ((src[pos] == ':') || (src[pos] == '#'))
        {
            write_pos = 0;
            end_pos = pos;
            port_pos = pos;
        }
    }

    if (src[0] != '.')
    {
        write_len = end_pos;
        memcpy(dest + write_pos, src, write_len);
        write_pos += write_len;
    }
    else
    {
        write_len = end_pos - 1;
        memcpy(dest + write_pos, src + 1, write_len);
        write_pos += write_len;
        dest[write_pos++] = '.';
    }

    if (port_pos != -1)
    {
        dest[write_pos++] = (src[port_pos] == ':')? '#' : ':';
        write_len = src_len - port_pos - 1;
        memcpy(dest + write_pos, src + port_pos + 1,  write_len);
        write_pos += write_len;
    }

    dest[write_pos] = '\0';

    return dest;
}

uint64_t Url::GetUrlHash() const
{
    return IsValid() ? common::MD5::Digest64(GetNormalizedUrl(NORMALIZE_FOR_DOCID)) : 0;
}

uint64_t Url::GetHostHash() const
{
    return IsValid() ? common::MD5::Digest64(m_host) : 0;
}

uint64_t Url::GetHostPortHash() const
{
    return IsValid() ? common::MD5::Digest64(GetHostPort()) : 0;
}

uint64_t Url::GetPortalPathHash() const
{
    return IsValid() ? common::MD5::Digest64(GetPortalPath()) : 0;
}

uint64_t Url::GetReversedSegmentPathHash() const
{
    return IsValid() ? common::MD5::Digest64(GetReversedSegmentPath()) : 0;
}

void Url::Clear()
{
    m_scheme = E_NO_PROTOCOL;
    m_host.clear();
    m_port = 0;
    m_path.clear();
    m_query.clear();
    m_has_query = false;

    m_absolute_path.clear();

    m_directory_pos_list.clear();
    m_query_pos_list.clear();
    m_resource_pos = std::string::npos;
    m_relative_path_start_pos = std::string::npos;
    m_portal_path_end_pos = std::string::npos;

    m_valid_schemes = 0;
}

bool Url::Preprocess(const char * in_url, const int in_url_len,
                     std::string * out_url,
                     bool decode_percent_encoded) const
{
    char ch, decoded_ch;
    int read_pos = 0;

    out_url->reserve(in_url_len);
    while (read_pos < in_url_len)
    {
        ch = in_url[read_pos];

        if ((ch == '%') && (read_pos + 2 < in_url_len)
            && isxdigit((unsigned char)(in_url[read_pos + 1]))
            && isxdigit((unsigned char)(in_url[read_pos + 2])))
        {
            decoded_ch = X2DIGITS_TO_NUM(in_url[read_pos + 1], in_url[read_pos + 2]);
            if ((unsigned int)decoded_ch < 0x20)
            {
                return false;
            }

            if (decode_percent_encoded)
            {
                ch = decoded_ch;

                if (URL_RESERVED_CHAR(ch) || (ch == '%'))
                {
                    out_url->append(in_url + read_pos, 3);
                    read_pos += 3;
                    continue;
                }
                else
                {
                    read_pos += 2;
                }
            }

            out_url->append(1, ch);
        }
        else if (ch != '&')
        {
            out_url->append(1, ch);
        }
        else if ((read_pos + 4 < in_url_len)
                 && (in_url[read_pos + 1] == 'a')
                 && (in_url[read_pos + 2] == 'm')
                 && (in_url[read_pos + 3] == 'p')
                 && (in_url[read_pos + 4] == ';'))
        {
            out_url->append(1, '&');
            read_pos += 5;
            continue;
        }
        else if ((read_pos + 3 < in_url_len)
                 && (in_url[read_pos + 1] == 'l')
                 && (in_url[read_pos + 2] == 't')
                 && (in_url[read_pos + 3] == ';'))
        {
            out_url->append(1, '<');
            read_pos += 4;
            continue;
        }
        else if ((read_pos + 3 < in_url_len)
                 && (in_url[read_pos + 1] == 'g')
                 && (in_url[read_pos + 2] == 't')
                 && (in_url[read_pos + 3] == ';'))
        {
            out_url->append(1, '>');
            read_pos += 4;
            continue;
        }
        else if ((read_pos + 5 < in_url_len)
                 && (in_url[read_pos + 1] == 'q')
                 && (in_url[read_pos + 2] == 'u')
                 && (in_url[read_pos + 3] == 'o')
                 && (in_url[read_pos + 4] == 't')
                 && (in_url[read_pos + 5] == ';'))
        {
            out_url->append(1, '"');
            read_pos += 6;
            continue;
        }
        else if ((read_pos + 5 < in_url_len)
                 && (in_url[read_pos + 1] == 'a')
                 && (in_url[read_pos + 2] == 'p')
                 && (in_url[read_pos + 3] == 'o')
                 && (in_url[read_pos + 4] == 's')
                 && (in_url[read_pos + 5] == ';'))
        {
            out_url->append(1, '\'');
            read_pos += 6;
            continue;
        }
        else
        {
            out_url->append(1, ch);
        }

        if (ch == '\\')
        {
            (*out_url)[out_url->length() - 1] = '/';
        }
        else if ((unsigned int)ch < 0x20)
        {
            return false;
        }

        read_pos++;

        if ((out_url->length() > 2)
            && ((*out_url)[out_url->length() - 1] == '/')
            && ((*out_url)[out_url->length() - 2] == '/')
            && ((*out_url)[out_url->length() - 3] != ':')
            && ((*out_url)[out_url->length() - 3] != '"'))
        {
            out_url->erase(out_url->length() - 1, 1);
        }
    }

    const char * start = out_url->c_str();
    const char * end = start + out_url->length() - 1;

    /// 去除首尾的空格
    if (TrimSpace(start, end) <= 0)
    {
        return false;
    }

    /// 去除首尾的引号
    if ((*start == '"') && (end > start) && (*end == '"'))
    {
        start++;
        end--;
    }

    /// 去除引号中的空格
    if (TrimSpace(start, end) <= 0)
    {
        return false;
    }

    if (start != out_url->c_str())
    {
        out_url->assign(start, end - start + 1);
    }
    else if (end != out_url->c_str() + out_url->length() - 1)
    {
        out_url->resize(end - start + 1);
    }

    return true;
}

bool Url::Parse(const char * url, const int url_len, EScheme default_scheme)
{
    bool has_scheme;
    const char * start = url;
    const char * end = url + url_len;

    if (!ParseScheme(start, end, default_scheme, has_scheme))
    {
        m_scheme = E_NO_PROTOCOL;
        return false;
    }

    if (has_scheme)
    {
        if (!ParseHost(start, end))
        {
            m_scheme = E_NO_PROTOCOL;
            return false;
        }
    }

    if (!ParsePath(start, end, has_scheme) || !ParseQuery(start, end))
    {
        m_scheme = E_NO_PROTOCOL;
        return false;
    }

    return true;
}

bool Url::ParseScheme(const char *& start, const char * end,
                      EScheme default_scheme, bool & has_scheme)
{
    has_scheme = false;

    if (((*start == 'h') || (*start == 'H'))
        && (start + 7 < end)
        && (STRNCASECMP(start + 1, "ttp", 3) == 0)
        && ((STRNCASECMP(start + 4, "://", 3) == 0)
            || (STRNCASECMP(start + 4, "s://", 4) == 0)))
    {
        if (*(start + 4) == ':')
        {
            SetSchemeAndPort(E_HTTP_PROTOCOL);
            has_scheme = true;
            start += 7;
        }
        else
        {
            SetSchemeAndPort(E_HTTPS_PROTOCOL);
            has_scheme = true;
            start += 8;
        }

        if (start >= end)
        {
            return false;
        }
    }
    else if ((*start == '/') && (start + 1 < end) && (*(start + 1) == '/'))
    {
        m_scheme = E_NO_PROTOCOL;
        has_scheme = true;
        start += 2;

        if (start >= end)
        {
            SetDefaultPath();
            return true;
        }
    }
    else
    {
        /// Scheme must start with alpha
        if (isalpha((unsigned char)*start))
        {
            int scan_len = end - start;
            scan_len = (scan_len < MAX_SCHEME_LENGTH) ? scan_len : MAX_SCHEME_LENGTH;
            for (int i = 0; i < scan_len; i++)
            {
                if (*(start + i) == ':')
                {
                    if ((start + i + 2 < end)
                        && (*(start + i + 1) == '/')
                        && (*(start + i + 2) == '/'))
                    {
                        int scheme_index = 2;   // start with 2 because we have checked "http(s)"
                        int scheme_count = sizeof(SCHEMES) / sizeof(SSchemeInfo);
                        for (; scheme_index < scheme_count; scheme_index++)
                        {
                            if (STRNCASECMP(start, SCHEMES[scheme_index].m_scheme_string.c_str(), i)
                                == 0)
                            {
                                SetSchemeAndPort(static_cast<EScheme>(scheme_index));
                                break;
                            }
                        }
                        if (scheme_index == scheme_count)
                        {
                            m_scheme = E_UNKNOWN_PROTOCOL;
                        }

                        has_scheme = true;
                        start += i + 3;

                        if (start >= end)
                        {
                            return false;
                        }
                    }
                    break;
                }
                else if (!SCHEME_SET(*(start + i)))
                {
                    break;
                }
            }
        }
    }

    if ((m_scheme == E_NO_PROTOCOL) && IsSchemeValid(default_scheme, m_valid_schemes))
    {
        has_scheme = true;
        SetSchemeAndPort(default_scheme);
    }

    return true;
}

bool Url::ParseHost(const char *& start, const char * end)
{
    const char * port_start;
    const char * host_start = start;
    while ((start < end) && (*start != ':') && (*start != '@')
           && (*start != '/') && (*start != '?') && (*start != '#'))
    {
        start++;
    }
    int host_len = start - host_start;

    if ((start < end) && (*start == ':'))
    {
        start++;
        port_start = start;
        bool has_port = true;
        while ((start < end) && (*start != '@') && (*start != '/')
               && (*start != '?') && (*start != '#'))
        {
            if (!isdigit((unsigned char)*start)
                || (start - port_start >= MAX_PORT_LENGTH))
            {
                has_port = false;
            }
            start++;
        }

        if (start >= end)
        {
            /// scheme://xxx:xxx
            if ((start > port_start) && has_port)
            {
                /// scheme://xxx:(port)
                char * end_ptr;
                if (!ParseNumber(port_start, &m_port, &end_ptr))
                {
                    return false;
                }
            }
            else
            {
                /// scheme://xxx: or scheme://xxx:(not port)
                return false;
            }
        }
        else if (*start == '@')
        {
            start++;
            host_start = start;
            while ((start < end) && (*start != ':') && (*start != '/')
                   && (*start != '?') && (*start != '#'))
            {
                start++;
            }
            host_len = start - host_start;

            if ((start < end) && (*start == ':'))
            {
                start++;
                port_start = start;
                while ((start < end) && (*start != '/') && (*start != '?') && (*start != '#'))
                {
                    if (!isdigit((unsigned char)*start)
                        || (start - port_start >= MAX_PORT_LENGTH))
                    {
                        /// scheme://xxx:xxx@xxx:(not port)
                        return false;
                    }
                    start++;
                }

                if (start > port_start)
                {
                    /// scheme://xxx:xxx@xxx:(port)
                    char * end_ptr;
                    if (!ParseNumber(port_start, &m_port, &end_ptr))
                    {
                        return false;
                    }
                }
                else
                {
                    /// scheme://xxx:xxx@xxx:
                    return false;
                }
            }
        }
        else
        {
            /// scheme://xxx:xxx/
            if ((start > port_start) && has_port)
            {
                /// scheme://xxx:(port)/
                char * end_ptr;
                if (!ParseNumber(port_start, &m_port, &end_ptr))
                {
                    return false;
                }
            }
            else
            {
                /// scheme://xxx:/ or scheme://xxx:(not port)/
                return false;
            }
        }
    }
    else if ((start < end) && (*start == '@'))
    {
        start++;
        host_start = start;
        while ((start < end) && (*start != ':') && (*start != '/')
               && (*start != '?') && (*start != '#'))
        {
            start++;
        }
        host_len = start - host_start;

        if ((start < end) && (*start == ':'))
        {
            start++;
            port_start = start;
            while ((start < end) && (*start != '/') && (*start != '?') && (*start != '#'))
            {
                if (!isdigit((unsigned char)*start)
                    || (start - port_start >= MAX_PORT_LENGTH))
                {
                    /// scheme://xxx@xxx:(not port)
                    return false;
                }
                start++;
            }

            if (start > port_start)
            {
                /// scheme://xxx@xxx:(port)
                char * end_ptr;
                if (!ParseNumber(port_start, &m_port, &end_ptr))
                {
                    return false;
                }
            }
            else
            {
                /// scheme://xxx@xxx:
                return false;
            }
        }
    }

    m_host.assign(host_start, host_len);

    for (std::string::size_type i = 0; i < m_host.length(); ++i)
    {
        if (i == 0)
        {
            if (!isalpha((unsigned char)m_host[i])
                && !isdigit((unsigned char)m_host[i])
                && !IsPercentEncoded(&m_host[i]))
            {
                return false;
            }
        }
        else
        {
            if (!DOMAIN_SET(m_host[i])
                && !IsPercentEncoded(&m_host[i]))
            {
                return false;
            }
        }
    }

    return true;
}

bool Url::ParsePath(const char *& start, const char * end, bool has_scheme)
{
    const char * start_pos = start;

    while ((start < end) && (*start != '?') && (*start != '#'))
    {
        start++;
    }

    if ((start == start_pos) && has_scheme)
    {
        SetDefaultPath();
    }
    else
    {
        m_path.assign(start_pos, start - start_pos);
    }

    return true;
}

bool Url::ParseQuery(const char *& start, const char * end)
{
    if ((start < end) && (*start == '?'))
    {
        m_has_query = true;
        start++;
        const char * start_pos = start;
        while ((start < end) && (*start != '#'))
        {
            start++;
        }
        m_query.assign(start_pos, start - start_pos);

        ParseQueryString(m_query, &m_query_pos_list);
    }

    return true;
}

void Url::ParseQueryString(const std::string & query, std::vector<QueryPos> * query_pos_list)
{
    bool searching_key = true;
    std::string::size_type read_pos = 0;
    QueryPos query_pos;
    query_pos.m_key_begin = read_pos;
    query_pos.m_key_end = std::string::npos;
    query_pos.m_value_begin = std::string::npos;
    query_pos.m_value_end = std::string::npos;

    while (read_pos < query.length())
    {
        if (searching_key && (query[read_pos] == '='))
        {
            query_pos.m_key_end = read_pos;
            query_pos.m_value_begin = read_pos + 1;
            searching_key = false;
        }
        else if (query[read_pos] == '&')
        {
            if (query_pos.m_key_end == std::string::npos)
            {
                query_pos.m_key_end = read_pos;
            }
            else if ((query_pos.m_value_begin != std::string::npos)
                     && (read_pos > query_pos.m_value_begin))
            {
                query_pos.m_value_end = read_pos;

                /// 去掉Value首尾的空格
                while ((query_pos.m_value_end > query_pos.m_value_begin)
                       && isspace((unsigned char)query[query_pos.m_value_end - 1]))
                {
                    query_pos.m_value_end--;
                }

                if (query_pos.m_value_end <= query_pos.m_value_begin)
                {
                    query_pos.m_value_end = std::string::npos;
                }
                else
                {
                    while ((query_pos.m_value_begin < query_pos.m_value_end)
                           && isspace((unsigned char)query[query_pos.m_value_begin]))
                    {
                        query_pos.m_value_begin++;
                    }
                }
            }

            /// 去掉Key首尾的空格
            while ((query_pos.m_key_end > query_pos.m_key_begin)
                   && isspace((unsigned char)query[query_pos.m_key_end - 1]))
            {
                query_pos.m_key_end--;
            }

            while ((query_pos.m_key_begin < query_pos.m_key_end)
                   && isspace((unsigned char)query[query_pos.m_key_begin]))
            {
                query_pos.m_key_begin++;
            }

            if (query_pos.m_key_end > query_pos.m_key_begin)
            {
                query_pos_list->push_back(query_pos);
            }
            query_pos.m_key_begin = read_pos + 1;
            query_pos.m_key_end = std::string::npos;
            query_pos.m_value_begin = std::string::npos;
            query_pos.m_value_end = std::string::npos;
            searching_key = true;
        }

        read_pos++;
    }

    if (query_pos.m_key_end == std::string::npos)
    {
        query_pos.m_key_end = read_pos;
    }
    else if ((query_pos.m_value_begin != std::string::npos)
             && (read_pos > query_pos.m_value_begin))
    {
        query_pos.m_value_end = read_pos;

        /// 去掉Value首尾的空格
        while ((query_pos.m_value_end > query_pos.m_value_begin)
               && isspace((unsigned char)query[query_pos.m_value_end - 1]))
        {
            query_pos.m_value_end--;
        }

        if (query_pos.m_value_end <= query_pos.m_value_begin)
        {
            query_pos.m_value_end = std::string::npos;
        }
        else
        {
            while ((query_pos.m_value_begin < query_pos.m_value_end)
                   && isspace((unsigned char)query[query_pos.m_value_begin]))
            {
                query_pos.m_value_begin++;
            }
        }
    }

    /// 去掉Key首尾的空格
    while ((query_pos.m_key_end > query_pos.m_key_begin)
           && isspace((unsigned char)query[query_pos.m_key_end - 1]))
    {
        query_pos.m_key_end--;
    }

    while ((query_pos.m_key_begin < query_pos.m_key_end)
           && isspace((unsigned char)query[query_pos.m_key_begin]))
    {
        query_pos.m_key_begin++;
    }

    if (query_pos.m_key_end > query_pos.m_key_begin)
    {
        query_pos_list->push_back(query_pos);
    }
}

/// 使用冒泡排序法排序查询参数，排序过程同时去除重复的参数对
void Url::SortQuery(const std::string & query, std::vector<QueryPos> * query_pos_list)
{
    int i, j;
    std::string::size_type pos_1, pos_2;
    bool exchange;
    int key_compare;
    QueryPos query_pos_temp;

    for (i = 0; i < static_cast<int>(query_pos_list->size()) - 1; i++)
    {
        exchange = false;
        for (j = static_cast<int>(query_pos_list->size()) - 2; j >= i; j--)
        {
            QueryPos & query_pos_1 = (*query_pos_list)[j];
            QueryPos & query_pos_2 = (*query_pos_list)[j + 1];

            key_compare = 0;
            pos_1 = query_pos_1.m_key_begin;
            pos_2 = query_pos_2.m_key_begin;
            while ((pos_1 < query_pos_1.m_key_end) && (pos_2 < query_pos_2.m_key_end)
                   && (tolower(query[pos_1]) == tolower(query[pos_2])))
            {
                if (key_compare == 0)
                {
                    if (query[pos_1] > query[pos_2])
                        key_compare = 1;
                    else if (query[pos_1] < query[pos_2])
                        key_compare = -1;
                    else
                        key_compare = 0;
                }
                pos_1++;
                pos_2++;
            }

            if ((pos_1 >= query_pos_1.m_key_end) && (pos_2 >= query_pos_2.m_key_end))
            {
                if (key_compare == 0)
                {
                    /// 删除重复的参数，只保留最后一个
                    query_pos_list->erase(query_pos_list->begin() + j);
                    continue;
                }
                else if (key_compare > 0)
                {
                    query_pos_temp = query_pos_1;
                    query_pos_1 = query_pos_2;
                    query_pos_2 = query_pos_temp;
                    exchange = true;
                    continue;
                }
            }

            if ((pos_1 < query_pos_1.m_key_end) && ((pos_2 >= query_pos_2.m_key_end)
                || (tolower(query[pos_1]) > tolower(query[pos_2]))))
            {
                query_pos_temp = query_pos_1;
                query_pos_1 = query_pos_2;
                query_pos_2 = query_pos_temp;
                exchange = true;
            }
        }
        if (!exchange) return;
    }
}

void Url::Recompose(int normalize_flag)
{
    /// 协议
    if (IsSchemeValid(m_scheme, m_valid_schemes))
    {
        SetSchemeAndPort(m_scheme);
        m_absolute_path = SCHEMES[m_scheme].m_scheme_string + "://";
    }

    /// 域名转小写
    for (std::string::size_type i = 0; i < m_host.length(); i++)
    {
        m_host[i] = tolower(m_host[i]);
    }

    /// 忽略域名末尾的'.'
    while (!m_host.empty() && (m_host[m_host.length() - 1] == '.'))
    {
        m_host.erase(m_host.length() - 1);
    }

    /// 域名
    m_absolute_path.append(m_host);
    if (HasExplicitPort())
    {
        m_absolute_path.append(1, ':');
        m_absolute_path.append(NumberToString(m_port));
    }

    /// 路径
    m_relative_path_start_pos = m_absolute_path.length();

    RemoveDotSegments(&m_path);

    std::string::size_type path_read_pos = 0;
    while (path_read_pos < m_path.length())
    {
        if (m_path[path_read_pos] == '/')
        {
            m_directory_pos_list.push_back(path_read_pos);
            m_resource_pos = path_read_pos + 1;
        }
        path_read_pos++;
    }
    if (m_resource_pos == m_path.length())
    {
        m_resource_pos = std::string::npos;
    }

    if ((normalize_flag & E_NORMALIZE_TRUNCATE_DEFAULT_PATH)
        && !IsDynamicPage() && (m_directory_pos_list.size() == 1)
        && IsDefaultPage(GetResourcePtr()))
    {
        m_path.resize(m_resource_pos);
        m_resource_pos = std::string::npos;
    }

    m_absolute_path.append(m_path);
    m_portal_path_end_pos = m_absolute_path.length();

    /// 查询
    if (m_has_query)
    {
        if (normalize_flag & E_NORMALIZE_SORT_QUERY)
        {
            SortQuery(m_query, &m_query_pos_list);
        }

        std::string recomposed_query;
        RecomposeQuery(m_query, m_query_pos_list, &recomposed_query);

        m_absolute_path.append(1, '?');
        m_absolute_path.append(recomposed_query);
    }
}

void Url::RecomposeQuery(const std::string & query, const std::vector<QueryPos> & query_pos_list,
                         std::string * recomposed_query)
{
    recomposed_query->reserve(query.length());

    std::vector<QueryPos>::const_iterator it;
    for (it = query_pos_list.begin(); it != query_pos_list.end(); ++it)
    {
        recomposed_query->append(query, it->m_key_begin, it->m_key_end - it->m_key_begin);

        if (it->m_value_begin != std::string::npos)
        {
            recomposed_query->append(1, '=');

            if (it->m_value_end != std::string::npos)
            {
                recomposed_query->append(query, it->m_value_begin,
                                         it->m_value_end - it->m_value_begin);
            }
        }

        recomposed_query->append(1, '&');
    }

    if (!recomposed_query->empty())
    {
        recomposed_query->erase(recomposed_query->length() - 1, 1);
    }
}

void Url::JoinPath(const std::string & path)
{
    std::string::size_type pos = path.rfind('/');

    if (pos != std::string::npos)
    {
        m_path = path.substr(0, pos + 1) + m_path;
    }
}

void Url::RemoveDotSegments(std::string * path)
{
    /***********************************************************************
    * a)
    * All occurrences of "./", where "." is a complete path
    * segment, are removed.
    ***********************************************************************/

    /**************************************************************************
    * b)
    * If the path ends with "." as a complete path segment, that "." is removed.
    ***************************************************************************/

    /***********************************************************
    * c)
    * All occurrences of "<segment>/../", where <segment> is a
    * complete path segment not equal to "..", are removed.
    * Removal of these path segments is performed iteratively,
    * removing the leftmost matching pattern on each iteration,
    * until no matching pattern remains.
    ************************************************************/

    /***********************************************************
    * d)
    * If the path ends with "<segment>/..", where <segment> is a
    * complete path segment not equal to "..", that
    * "<segment>/.." is removed.
    ***********************************************************/

    std::string::size_type read_pos, write_pos;
    read_pos = write_pos = 0;

    while (read_pos < path->length())
    {
        if (((*path)[read_pos] == '.')
            && ((write_pos == 0) || ((*path)[write_pos - 1] == '/')))
        {
            if (read_pos + 1 < path->length())
            {
                if ((*path)[read_pos + 1] == '.')
                {
                    if (read_pos + 2 < path->length())
                    {
                        if ((*path)[read_pos + 2] == '/')
                        {
                            /// <segment>/../
                            if (write_pos > 0)
                            {
                                write_pos--;
                            }
                            while ((write_pos >= 1) && ((*path)[write_pos - 1] != '/'))
                            {
                                write_pos--;
                            }
                            read_pos += 3;
                            if (write_pos <= 0)
                            {
                                write_pos = 0;
                                (*path)[write_pos++] = '/';
                            }
                            continue;
                        }
                    }
                    else
                    {
                        /// <segment>/..
                        if (write_pos > 0)
                        {
                            write_pos--;
                        }
                        while ((write_pos >= 1) && ((*path)[write_pos - 1] != '/'))
                        {
                            write_pos--;
                        }
                        break;
                    }
                }
                else if ((*path)[read_pos + 1] == '/')
                {
                    /// <segment>/./
                    read_pos += 2;
                    continue;
                }
            }
            else
            {
                /// <segment>/.
                break;
            }
        }

        (*path)[write_pos] = (*path)[read_pos];
        write_pos++;
        read_pos++;
    }

    path->resize(write_pos);
}

void Url::SetSchemeAndPort(EScheme scheme)
{
    m_scheme = scheme;

    if ((m_port == 0) && IsSchemeValid(m_scheme, m_valid_schemes))
    {
        m_port = SCHEMES[m_scheme].m_default_port;
    }
}

bool Url::HasExplicitPort() const
{
    return ((m_port != 0)
            && IsSchemeValid(m_scheme, m_valid_schemes)
            && (m_port != SCHEMES[m_scheme].m_default_port));
}

bool Url::IsDefaultPage(const char * resource)
{
    if (resource == NULL)
    {
        return false;
    }

    const char * DEFAULT_PAGE_PREFIX[] = { "index.", "default." };
    const char * DEFAULT_PAGE_SUFFIX[] = { "html", "htm", "shtml" };

    size_t index, count, offset;

    count = sizeof(DEFAULT_PAGE_PREFIX) / sizeof(DEFAULT_PAGE_PREFIX[0]);
    for (index = 0; index < count; index++)
    {
        offset = strlen(DEFAULT_PAGE_PREFIX[index]);
        if (STRNCASECMP(resource, DEFAULT_PAGE_PREFIX[index], offset) == 0)
        {
            break;
        }
    }

    if (index < count)
    {
        count = sizeof(DEFAULT_PAGE_SUFFIX) / sizeof(DEFAULT_PAGE_SUFFIX[0]);
        for (index = 0; index < count; index++)
        {
            if (STRCASECMP(resource + offset, DEFAULT_PAGE_SUFFIX[index]) == 0)
            {
                return true;
            }
        }
    }

    return false;
}

int Url::TrimSpace(const char *& start, const char *& end)
{
    if ((start == NULL) || (end == NULL))
    {
        return -1;
    }

    while (end >= start)
    {
        if (isspace((unsigned char)(*end)))
        {
            end--;
        }
        else if (((end - 2) >= start)
                 && IsPercentEncoded(end - 2)
                 && IS_PERCENT_ENCODED_SPACE(*(end - 1), *end))
        {
            end -= 3;
        }
        else
        {
            break;
        }
    }

    while (start <= end)
    {
        if (isspace((unsigned char)(*start)))
        {
            start++;
        }
        else if (((start + 2) <= end)
                 && IsPercentEncoded(start)
                 && IS_PERCENT_ENCODED_SPACE(*(start + 1), *(start + 2)))
        {
            start += 3;
        }
        else
        {
            break;
        }
    }

    return end - start + 1;
}

} // namespace url
} // namespace web
