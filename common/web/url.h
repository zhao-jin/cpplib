// Copyright (c) 2011, Tencent Inc. All rights reserved.
// Author:  Wen Jie <welkinwen@tencent.com>
// Created: 2011-04-20

#ifndef COMMON_WEB_URL_H
#define COMMON_WEB_URL_H

#include <inttypes.h>
#include <string.h>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "common/base/deprecate.h"

namespace web {
namespace url {

/////////////////////////////////////////////////////
//                Consts Defination                //
/////////////////////////////////////////////////////

/// Url长度限制
DEPRECATED
const int MAX_URL_LENGTH      = 1024;
DEPRECATED
const int MAX_HOST_LENGTH     = 50;
DEPRECATED_BY(MAX_HOST_LENGTH)
const int MAX_DOMAIN_LENGTH   = 50;
DEPRECATED
const int MAX_PATH_LENGTH     = 480;
DEPRECATED
const int MAX_QUERY_LENGTH    = 480;
DEPRECATED
const int MAX_DIRECTORY_COUNT = 20;
DEPRECATED
const int MAX_QUERY_COUNT     = 20;

/// 协议类型枚举
enum EScheme
{
    E_UNKNOWN_PROTOCOL = -2,
    E_NO_PROTOCOL = -1,
    E_HTTP_PROTOCOL = 0,
    E_HTTPS_PROTOCOL = 1,
    E_FTP_PROTOCOL = 2,
};

const int VALID_SCHEMES_FOR_DOWNLOAD = (1 << E_HTTP_PROTOCOL)
                                     | (1 << E_HTTPS_PROTOCOL)
                                     | (1 << E_FTP_PROTOCOL);
const int VALID_SCHEMES_FOR_INDEX = (1 << E_HTTP_PROTOCOL)
                                  | (1 << E_HTTPS_PROTOCOL);

/// 规范化类型
enum ENormalizeType
{
    E_NORMALIZE_PREPROCESS = 0x01,
    E_NORMALIZE_DECODE_HEXADECIMAL = 0x02,
    E_NORMALIZE_TRUNCATE_DEFAULT_PATH = 0x04,
    E_NORMALIZE_SORT_QUERY = 0x08,
    E_NORMALIZE_LOWER_CASE = 0x10,
    E_NORMALIZE_TRUNCATE_LAST_SLASH = 0x20,
};

const int NORMALIZE_FOR_DOWNLOAD = E_NORMALIZE_PREPROCESS;
const int NORMALIZE_FOR_INDEX = E_NORMALIZE_PREPROCESS | E_NORMALIZE_TRUNCATE_DEFAULT_PATH
                              | E_NORMALIZE_SORT_QUERY;
const int NORMALIZE_FOR_DOCID = E_NORMALIZE_PREPROCESS | E_NORMALIZE_TRUNCATE_DEFAULT_PATH
                              | E_NORMALIZE_DECODE_HEXADECIMAL | E_NORMALIZE_SORT_QUERY
                              | E_NORMALIZE_LOWER_CASE | E_NORMALIZE_TRUNCATE_LAST_SLASH;

DEPRECATED
const int INVALID_POS = -1;

struct QueryPos
{
    std::string::size_type m_key_begin;
    std::string::size_type m_key_end;
    std::string::size_type m_value_begin;
    std::string::size_type m_value_end;
};

/////////////////////////////////////////////////////
//                Class Declaration                //
/////////////////////////////////////////////////////

////////////////// Url //////////////////

class Url
{
public:
    Url();

    /// 由URL字符串构造Url对象，构造完成可通过IsValid()判断URL是否解析成功
    /// @param  url            URL字符串
    /// @param  url_len        URL字符串长度
    /// @param  default_scheme 默认协议类型，当URL没有明确的协议类型时使用该值
    /// @param  normalize_flag 归一化标志：NORMALIZE_FOR_DOWNLOAD适用于网络下载
    ///                                    NORMALIZE_FOR_INDEX适用于检索展现
    ///                                    NORMALIZE_FOR_DOCID适用于计算DocID
    /// @param  valid_schemes  有效协议集：VALID_SCHEMES_FOR_DOWNLOAD为下载系统认定有效的协议集
    ///                                    VALID_SCHEMES_FOR_INDEX为索引系统认定有效的协议集
    Url(const char * url, const int url_len,
        EScheme default_scheme = E_NO_PROTOCOL,
        int normalize_flag = NORMALIZE_FOR_INDEX,
        int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// 由URL字符串和基准URL字符串构造Url对象，构造完成可通过IsValid()判断URL是否解析成功
    /// @param  url            URL字符串
    /// @param  url_len        URL字符串长度
    /// @param  base_url       基准URL字符串
    /// @param  base_url_len   基准URL字符串长度
    /// @param  normalize_flag 归一化标志：NORMALIZE_FOR_DOWNLOAD适用于网络下载
    ///                                    NORMALIZE_FOR_INDEX适用于检索展现
    ///                                    NORMALIZE_FOR_DOCID适用于计算DocID
    /// @param  valid_schemes  有效协议集：VALID_SCHEMES_FOR_DOWNLOAD为下载系统认定有效的协议集
    ///                                    VALID_SCHEMES_FOR_INDEX为索引系统认定有效的协议集
    Url(const char * url, const int url_len,
        const char * base_url, const int base_url_len,
        int normalize_flag = NORMALIZE_FOR_INDEX,
        int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// 由URL字符串和基准Url对象构造Url对象，构造完成可通过IsValid()判断URL是否解析成功
    /// @param  url            URL字符串
    /// @param  url_len        URL字符串长度
    /// @param  base_url       基准Url对象
    /// @param  normalize_flag 归一化标志：NORMALIZE_FOR_DOWNLOAD适用于网络下载
    ///                                    NORMALIZE_FOR_INDEX适用于检索展现
    ///                                    NORMALIZE_FOR_DOCID适用于计算DocID
    /// @param  valid_schemes  有效协议集：VALID_SCHEMES_FOR_DOWNLOAD为下载系统认定有效的协议集
    ///                                    VALID_SCHEMES_FOR_INDEX为索引系统认定有效的协议集
    Url(const char * url, const int url_len,
        const Url & base_url,
        int normalize_flag = NORMALIZE_FOR_INDEX,
        int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// 由URL字符串构造Url对象
    /// @param  url            URL字符串
    /// @param  url_len        URL字符串长度
    /// @param  default_scheme 默认协议类型，当URL没有明确的协议类型时使用该值
    /// @param  normalize_flag 归一化标志：NORMALIZE_FOR_DOWNLOAD适用于网络下载
    ///                                    NORMALIZE_FOR_INDEX适用于检索展现
    ///                                    NORMALIZE_FOR_DOCID适用于计算DocID
    /// @param  valid_schemes  有效协议集：VALID_SCHEMES_FOR_DOWNLOAD为下载系统认定有效的协议集
    ///                                    VALID_SCHEMES_FOR_INDEX为索引系统认定有效的协议集
    /// @retval true           解析URL成功
    /// @retval false          解析URL失败
    bool Load(const char * url, const int url_len,
              EScheme default_scheme = E_NO_PROTOCOL,
              int normalize_flag = NORMALIZE_FOR_INDEX,
              int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// 由URL字符串和基准URL字符串构造Url对象
    /// @param  url            URL字符串
    /// @param  url_len        URL字符串长度
    /// @param  base_url       基准URL字符串
    /// @param  base_url_len   基准URL字符串长度
    /// @param  normalize_flag 归一化标志：NORMALIZE_FOR_DOWNLOAD适用于网络下载
    ///                                    NORMALIZE_FOR_INDEX适用于检索展现
    ///                                    NORMALIZE_FOR_DOCID适用于计算DocID
    /// @param  valid_schemes  有效协议集：VALID_SCHEMES_FOR_DOWNLOAD为下载系统认定有效的协议集
    ///                                    VALID_SCHEMES_FOR_INDEX为索引系统认定有效的协议集
    /// @retval true           解析URL成功
    /// @retval false          解析URL失败
    bool Load(const char * url, const int url_len,
              const char * base_url, const int base_url_len,
              int normalize_flag = NORMALIZE_FOR_INDEX,
              int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// 由URL字符串和基准Url对象构造Url对象
    /// @param  url            URL字符串
    /// @param  url_len        URL字符串长度
    /// @param  base_url       基准Url对象
    /// @param  normalize_flag 归一化标志：NORMALIZE_FOR_DOWNLOAD适用于网络下载
    ///                                    NORMALIZE_FOR_INDEX适用于检索展现
    ///                                    NORMALIZE_FOR_DOCID适用于计算DocID
    /// @param  valid_schemes  有效协议集：VALID_SCHEMES_FOR_DOWNLOAD为下载系统认定有效的协议集
    ///                                    VALID_SCHEMES_FOR_INDEX为索引系统认定有效的协议集
    /// @retval true           解析URL成功
    /// @retval false          解析URL失败
    bool Load(const char * url, const int url_len,
              const Url & base_url,
              int normalize_flag = NORMALIZE_FOR_INDEX,
              int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// 由反转URL构造Url对象
    /// @param  reversed_url     反转URL
    /// @param  reversed_url_len 反转URL长度
    /// @param  default_scheme   默认协议类型，当URL没有明确的协议类型时使用该值
    /// @param  normalize_flag   归一化标志：NORMALIZE_FOR_DOWNLOAD适用于网络下载
    ///                                      NORMALIZE_FOR_INDEX适用于检索展现
    ///                                      NORMALIZE_FOR_DOCID适用于计算DocID
    /// @param  valid_schemes    有效协议集：VALID_SCHEMES_FOR_DOWNLOAD为下载系统认定有效的协议集
    ///                                      VALID_SCHEMES_FOR_INDEX为索引系统认定有效的协议集
    /// @retval true             解析URL成功
    /// @retval false            解析URL失败
    bool LoadReversedUrl(const void * reversed_url, const int reversed_url_len,
                         EScheme default_scheme = E_NO_PROTOCOL,
                         int normalize_flag = 0,
                         int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// 由反转URL string构造Url对象
    /// @param  reversed_url   反转URL string
    /// @param  default_scheme 默认协议类型，当URL没有明确的协议类型时使用该值
    /// @param  normalize_flag 归一化标志：NORMALIZE_FOR_DOWNLOAD适用于网络下载
    ///                                    NORMALIZE_FOR_INDEX适用于检索展现
    ///                                    NORMALIZE_FOR_DOCID适用于计算DocID
    /// @param  valid_schemes  有效协议集：VALID_SCHEMES_FOR_DOWNLOAD为下载系统认定有效的协议集
    ///                                    VALID_SCHEMES_FOR_INDEX为索引系统认定有效的协议集
    /// @retval true           解析URL成功
    /// @retval false          解析URL失败
    inline bool LoadReversedUrl(const std::string & reversed_url,
                                EScheme default_scheme = E_NO_PROTOCOL,
                                int normalize_flag = 0,
                                int valid_schemes = VALID_SCHEMES_FOR_INDEX)
    {
        return LoadReversedUrl(reversed_url.c_str(), reversed_url.length(),
                               default_scheme, normalize_flag, valid_schemes);
    }

    /// 清空对象
    void Clear();

    /// URL是否有效
    inline bool IsValid() const
    {
        return (IsSchemeValid(m_scheme, m_valid_schemes) && !m_host.empty() && !m_path.empty());
    }

    /// URL是否为动态页
    inline bool IsDynamicPage() const
    {
        return m_has_query;
    }

    /// URL是否为首页
    inline bool IsHomePage() const
    {
        return (!IsDynamicPage() && (m_path == "/"));
    }

    /// 获取规范化后的URL
    std::string GetNormalizedUrl(int normalize_flag = NORMALIZE_FOR_DOCID) const;

    /// 获取反转URL，模式为：<reversed domain>[#port]<"scheme"><relative path>
    bool GetReversedUrl(void * reversed_url, int & reversed_url_len,
                        int normalize_flag = NORMALIZE_FOR_DOCID) const;

    std::string GetReversedUrl(int normalize_flag = NORMALIZE_FOR_DOCID) const;

    /// 获取绝对路径
    inline const std::string & GetAbsolutePath() const
    {
        return m_absolute_path;
    }

    inline const char * GetAbsolutePathPtr() const
    {
        return m_absolute_path.c_str();
    }

    /// 获取相对路径
    inline std::string GetRelativePath() const
    {
        return (m_relative_path_start_pos == std::string::npos) ?
               std::string("") : m_absolute_path.substr(m_relative_path_start_pos);
    }

    inline const char * GetRelativePathPtr() const
    {
        return (m_relative_path_start_pos == std::string::npos) ?
               NULL : (m_absolute_path.c_str() + m_relative_path_start_pos);
    }

    /// 获取入口路径
    inline std::string GetPortalPath() const
    {
        return (m_portal_path_end_pos == std::string::npos) ?
               std::string("") : m_absolute_path.substr(0, m_portal_path_end_pos);
    }

    /// 获取片段路径
    std::string GetSegmentPath() const;

    /// 获取反转片段路径
    std::string GetReversedSegmentPath() const;

    /// 获取首页URL
    inline std::string GetHomePageURL() const
    {
        return (m_relative_path_start_pos == std::string::npos) ?
               std::string("") : m_absolute_path.substr(0, m_relative_path_start_pos + 1);
    }

    /// 获取协议
    inline EScheme GetScheme() const
    {
        return m_scheme;
    }

    /// 获取协议字符串
    std::string GetSchemeString() const;

    /// 获取域名
    inline const std::string & GetHost() const
    {
        return m_host;
    }

    inline const char * GetHostPtr() const
    {
        return m_host.c_str();
    }

    DEPRECATED_BY(GetHost)
    inline const std::string & GetDomain() const
    {
        return GetHost();
    }

    /// 获取带端口的域名
    std::string GetHostPort() const;

    DEPRECATED_BY(GetHostPort)
    inline std::string GetDomainWithPort() const
    {
        return GetHostPort();
    }

    /// 获取反转域名
    inline std::string GetReversedHost() const
    {
        return ReverseDomain(m_host);
    }

    DEPRECATED_BY(GetReversedHost)
    inline std::string GetReversedDomain() const
    {
        return GetReversedHost();
    }

    /// 获取带端口的反转域名
    inline std::string GetReversedHostPort() const
    {
        return ReverseDomain(GetHostPort());
    }

    DEPRECATED_BY(GetReversedHostPort)
    inline std::string GetReversedDomainWithPort() const
    {
        return GetReversedHostPort();
    }

    /// 获取端口
    inline uint16_t GetPort() const
    {
        return m_port;
    }

    /// 获取路径
    inline const std::string & GetPath() const
    {
        return m_path;
    }

    inline const char * GetPathPtr() const
    {
        return m_path.c_str();
    }

    /// 获取查询
    void GetQuerys(std::map<std::string, std::string> & querys) const;

    /// 获取目录
    std::string GetDirectory() const;

    /// 获取所有层级目录
    void GetDirectories(std::vector<std::string> & directories) const;

    /// 获取资源名
    inline std::string GetResource() const
    {
        return (m_resource_pos == std::string::npos) ?
               std::string("") : m_path.substr(m_resource_pos);
    }

    inline const char * GetResourcePtr() const
    {
        return (m_resource_pos == std::string::npos) ? NULL : (m_path.c_str() + m_resource_pos);
    }

    /// 获取资源后缀名
    std::string GetResourceSuffix() const;

    const char * GetResourceSuffixPtr() const;

    /// 获取URL的路径深度（目录层级）
    inline unsigned int GetPathDepth() const
    {
        return m_directory_pos_list.size();
    }

public:
    /// 对URL进行编码，默认不编码保留字
    /// 若需要将URL作为文件名，可选择编码保留字
    /// 若需要保留显示中文字符，可选择不编码扩展字符集
    enum
    {
        E_ENCODE_RESERVED_CHAR = 0x01,          ///< 编码保留字
        E_NOT_ENCODE_EXTENDED_CHAR_SET = 0x02   ///< 不编码扩展字符集(ASCII值大于127的字符)
    };
    static std::string EncodeUrl(const std::string & url, int flag = 0);

    /// 对URL进行解码，默认不解码保留字和百分号字符
    enum
    {
        E_DECODE_RESERVED_CHAR = 0x10,          ///< 解码保留字
        E_DECODE_PERCENT_SIGN_CHAR = 0x20       ///< 解码百分号字符
    };
    static std::string DecodeUrl(const std::string & url, int flag = 0);

    /// 反转域名，支持带端口的域名互转：domain[:port] <==> reversed domain[#port]
    /// @param  domain 要反转的域名
    /// @return 反转后的域名
    static std::string ReverseDomain(const std::string & domain);

    /// 反转域名，支持带端口的域名互转：domain[:port] <==> reversed domain[#port]
    /// @param  src     要反转的域名字符串
    /// @param  src_len 要反转的域名字符串长度
    /// @param  dest    用来存放反转后域名的字符串
    /// @return 指向反转后域名字符串的首指针
    static char * ReverseDomain(const char * src, const int src_len, char * dest);

public:
    /// 获取URL哈希
    uint64_t GetUrlHash() const;

    /// 获取域名哈希
    uint64_t GetHostHash() const;
    uint64_t GetHostPortHash() const;

    DEPRECATED_BY(GetHostHash)
    inline uint64_t GetDomainHash() const
    {
        return GetHostHash();
    }

    DEPRECATED_BY(GetHostPortHash)
    inline uint64_t GetDomainWithPortHash() const
    {
        return GetHostPortHash();
    }

    /// 获取入口路径哈希
    uint64_t GetPortalPathHash() const;

    /// 获取反转片段路径哈希
    uint64_t GetReversedSegmentPathHash() const;

private:
    /// 预处理
    bool Preprocess(const char * in_url, const int in_url_len,
                    std::string * out_url, bool decode_percent_encoded) const;

    /// 解析URL
    bool Parse(const char * url, const int url_len, EScheme default_scheme);

    /// 解析协议
    bool ParseScheme(const char *& start, const char * end,
                     EScheme default_scheme, bool & has_scheme);

    /// 解析域名
    bool ParseHost(const char *& start, const char * end);

    /// 解析路径
    bool ParsePath(const char *& start, const char * end, bool has_scheme);

    /// 解析查询
    bool ParseQuery(const char *& start, const char * end);

    /// 解析查询字符串
    static void ParseQueryString(const std::string & query,
                                 std::vector<QueryPos> * query_pos_list);

    /// 排序查询
    static void SortQuery(const std::string & query, std::vector<QueryPos> * query_pos_list);

    /// 重组URL
    void Recompose(int normalize_flag);

    /// 重组查询
    static void RecomposeQuery(const std::string & query,
                               const std::vector<QueryPos> & query_pos_list,
                               std::string * recomposed_query);

    /// 拼接路径
    void JoinPath(const std::string & path);

    /// 修整路径
    static void RemoveDotSegments(std::string * path);

    /// 复制域名信息
    inline void CopyHostInfo(const Url & url)
    {
        m_host = url.m_host;
        m_port = url.m_port;
    }

    /// 复制路径信息
    inline void CopyPathInfo(const Url & url)
    {
        m_path = url.m_path;
    }

    /// 复制查询信息
    inline void CopyQueryInfo(const Url & url)
    {
        m_query = url.m_query;
        m_query_pos_list = url.m_query_pos_list;
        m_has_query = url.m_has_query;
    }

    /// 设置协议和端口
    void SetSchemeAndPort(EScheme scheme);

    /// 设置默认路径
    inline void SetDefaultPath()
    {
        m_path = "/";
    }

    /// 是否为有效协议
    static inline bool IsSchemeValid(EScheme scheme, int valid_schemes)
    {
        if ((scheme != E_NO_PROTOCOL) && (scheme != E_UNKNOWN_PROTOCOL))
        {
            return ((1 << scheme) & valid_schemes);
        }

        return false;
    }

    /// 是否有显式端口
    bool HasExplicitPort() const;

    /// 是否为默认页
    static bool IsDefaultPage(const char * resource);

    /// 判断字符串是否为百分号编码
    static inline bool IsPercentEncoded(const char * str)
    {
        if ((str[0] != '%')
            || !isxdigit((unsigned char)str[1])
            || !isxdigit((unsigned char)str[2]))
        {
            return false;
        }

        return true;
    }

    /// 去除首尾两端的空格
    static int TrimSpace(const char *& start, const char *& end);

private:
    /// <scheme>://<authority>/<path>;<params>?<query>#<fragment>
    EScheme m_scheme;                   ///< 协议
    std::string m_host;                 ///< 域名
    uint16_t m_port;                    ///< 端口
    std::string m_path;                 ///< 路径
    std::string m_query;                ///< 查询
    bool m_has_query;                   ///< 是否有查询

    std::string m_absolute_path;

    std::vector<std::string::size_type> m_directory_pos_list;
    std::vector<QueryPos> m_query_pos_list;
    std::string::size_type m_resource_pos;
    std::string::size_type m_relative_path_start_pos;
    std::string::size_type m_portal_path_end_pos;

    int m_valid_schemes;
};

DEPRECATED_BY(Url) typedef Url CUrl;

} // namespace url
} // namespace web

#endif // COMMON_WEB_URL_H
