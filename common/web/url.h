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

/// Url��������
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

/// Э������ö��
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

/// �淶������
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

    /// ��URL�ַ�������Url���󣬹�����ɿ�ͨ��IsValid()�ж�URL�Ƿ�����ɹ�
    /// @param  url            URL�ַ���
    /// @param  url_len        URL�ַ�������
    /// @param  default_scheme Ĭ��Э�����ͣ���URLû����ȷ��Э������ʱʹ�ø�ֵ
    /// @param  normalize_flag ��һ����־��NORMALIZE_FOR_DOWNLOAD��������������
    ///                                    NORMALIZE_FOR_INDEX�����ڼ���չ��
    ///                                    NORMALIZE_FOR_DOCID�����ڼ���DocID
    /// @param  valid_schemes  ��ЧЭ�鼯��VALID_SCHEMES_FOR_DOWNLOADΪ����ϵͳ�϶���Ч��Э�鼯
    ///                                    VALID_SCHEMES_FOR_INDEXΪ����ϵͳ�϶���Ч��Э�鼯
    Url(const char * url, const int url_len,
        EScheme default_scheme = E_NO_PROTOCOL,
        int normalize_flag = NORMALIZE_FOR_INDEX,
        int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// ��URL�ַ����ͻ�׼URL�ַ�������Url���󣬹�����ɿ�ͨ��IsValid()�ж�URL�Ƿ�����ɹ�
    /// @param  url            URL�ַ���
    /// @param  url_len        URL�ַ�������
    /// @param  base_url       ��׼URL�ַ���
    /// @param  base_url_len   ��׼URL�ַ�������
    /// @param  normalize_flag ��һ����־��NORMALIZE_FOR_DOWNLOAD��������������
    ///                                    NORMALIZE_FOR_INDEX�����ڼ���չ��
    ///                                    NORMALIZE_FOR_DOCID�����ڼ���DocID
    /// @param  valid_schemes  ��ЧЭ�鼯��VALID_SCHEMES_FOR_DOWNLOADΪ����ϵͳ�϶���Ч��Э�鼯
    ///                                    VALID_SCHEMES_FOR_INDEXΪ����ϵͳ�϶���Ч��Э�鼯
    Url(const char * url, const int url_len,
        const char * base_url, const int base_url_len,
        int normalize_flag = NORMALIZE_FOR_INDEX,
        int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// ��URL�ַ����ͻ�׼Url������Url���󣬹�����ɿ�ͨ��IsValid()�ж�URL�Ƿ�����ɹ�
    /// @param  url            URL�ַ���
    /// @param  url_len        URL�ַ�������
    /// @param  base_url       ��׼Url����
    /// @param  normalize_flag ��һ����־��NORMALIZE_FOR_DOWNLOAD��������������
    ///                                    NORMALIZE_FOR_INDEX�����ڼ���չ��
    ///                                    NORMALIZE_FOR_DOCID�����ڼ���DocID
    /// @param  valid_schemes  ��ЧЭ�鼯��VALID_SCHEMES_FOR_DOWNLOADΪ����ϵͳ�϶���Ч��Э�鼯
    ///                                    VALID_SCHEMES_FOR_INDEXΪ����ϵͳ�϶���Ч��Э�鼯
    Url(const char * url, const int url_len,
        const Url & base_url,
        int normalize_flag = NORMALIZE_FOR_INDEX,
        int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// ��URL�ַ�������Url����
    /// @param  url            URL�ַ���
    /// @param  url_len        URL�ַ�������
    /// @param  default_scheme Ĭ��Э�����ͣ���URLû����ȷ��Э������ʱʹ�ø�ֵ
    /// @param  normalize_flag ��һ����־��NORMALIZE_FOR_DOWNLOAD��������������
    ///                                    NORMALIZE_FOR_INDEX�����ڼ���չ��
    ///                                    NORMALIZE_FOR_DOCID�����ڼ���DocID
    /// @param  valid_schemes  ��ЧЭ�鼯��VALID_SCHEMES_FOR_DOWNLOADΪ����ϵͳ�϶���Ч��Э�鼯
    ///                                    VALID_SCHEMES_FOR_INDEXΪ����ϵͳ�϶���Ч��Э�鼯
    /// @retval true           ����URL�ɹ�
    /// @retval false          ����URLʧ��
    bool Load(const char * url, const int url_len,
              EScheme default_scheme = E_NO_PROTOCOL,
              int normalize_flag = NORMALIZE_FOR_INDEX,
              int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// ��URL�ַ����ͻ�׼URL�ַ�������Url����
    /// @param  url            URL�ַ���
    /// @param  url_len        URL�ַ�������
    /// @param  base_url       ��׼URL�ַ���
    /// @param  base_url_len   ��׼URL�ַ�������
    /// @param  normalize_flag ��һ����־��NORMALIZE_FOR_DOWNLOAD��������������
    ///                                    NORMALIZE_FOR_INDEX�����ڼ���չ��
    ///                                    NORMALIZE_FOR_DOCID�����ڼ���DocID
    /// @param  valid_schemes  ��ЧЭ�鼯��VALID_SCHEMES_FOR_DOWNLOADΪ����ϵͳ�϶���Ч��Э�鼯
    ///                                    VALID_SCHEMES_FOR_INDEXΪ����ϵͳ�϶���Ч��Э�鼯
    /// @retval true           ����URL�ɹ�
    /// @retval false          ����URLʧ��
    bool Load(const char * url, const int url_len,
              const char * base_url, const int base_url_len,
              int normalize_flag = NORMALIZE_FOR_INDEX,
              int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// ��URL�ַ����ͻ�׼Url������Url����
    /// @param  url            URL�ַ���
    /// @param  url_len        URL�ַ�������
    /// @param  base_url       ��׼Url����
    /// @param  normalize_flag ��һ����־��NORMALIZE_FOR_DOWNLOAD��������������
    ///                                    NORMALIZE_FOR_INDEX�����ڼ���չ��
    ///                                    NORMALIZE_FOR_DOCID�����ڼ���DocID
    /// @param  valid_schemes  ��ЧЭ�鼯��VALID_SCHEMES_FOR_DOWNLOADΪ����ϵͳ�϶���Ч��Э�鼯
    ///                                    VALID_SCHEMES_FOR_INDEXΪ����ϵͳ�϶���Ч��Э�鼯
    /// @retval true           ����URL�ɹ�
    /// @retval false          ����URLʧ��
    bool Load(const char * url, const int url_len,
              const Url & base_url,
              int normalize_flag = NORMALIZE_FOR_INDEX,
              int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// �ɷ�תURL����Url����
    /// @param  reversed_url     ��תURL
    /// @param  reversed_url_len ��תURL����
    /// @param  default_scheme   Ĭ��Э�����ͣ���URLû����ȷ��Э������ʱʹ�ø�ֵ
    /// @param  normalize_flag   ��һ����־��NORMALIZE_FOR_DOWNLOAD��������������
    ///                                      NORMALIZE_FOR_INDEX�����ڼ���չ��
    ///                                      NORMALIZE_FOR_DOCID�����ڼ���DocID
    /// @param  valid_schemes    ��ЧЭ�鼯��VALID_SCHEMES_FOR_DOWNLOADΪ����ϵͳ�϶���Ч��Э�鼯
    ///                                      VALID_SCHEMES_FOR_INDEXΪ����ϵͳ�϶���Ч��Э�鼯
    /// @retval true             ����URL�ɹ�
    /// @retval false            ����URLʧ��
    bool LoadReversedUrl(const void * reversed_url, const int reversed_url_len,
                         EScheme default_scheme = E_NO_PROTOCOL,
                         int normalize_flag = 0,
                         int valid_schemes = VALID_SCHEMES_FOR_INDEX);

    /// �ɷ�תURL string����Url����
    /// @param  reversed_url   ��תURL string
    /// @param  default_scheme Ĭ��Э�����ͣ���URLû����ȷ��Э������ʱʹ�ø�ֵ
    /// @param  normalize_flag ��һ����־��NORMALIZE_FOR_DOWNLOAD��������������
    ///                                    NORMALIZE_FOR_INDEX�����ڼ���չ��
    ///                                    NORMALIZE_FOR_DOCID�����ڼ���DocID
    /// @param  valid_schemes  ��ЧЭ�鼯��VALID_SCHEMES_FOR_DOWNLOADΪ����ϵͳ�϶���Ч��Э�鼯
    ///                                    VALID_SCHEMES_FOR_INDEXΪ����ϵͳ�϶���Ч��Э�鼯
    /// @retval true           ����URL�ɹ�
    /// @retval false          ����URLʧ��
    inline bool LoadReversedUrl(const std::string & reversed_url,
                                EScheme default_scheme = E_NO_PROTOCOL,
                                int normalize_flag = 0,
                                int valid_schemes = VALID_SCHEMES_FOR_INDEX)
    {
        return LoadReversedUrl(reversed_url.c_str(), reversed_url.length(),
                               default_scheme, normalize_flag, valid_schemes);
    }

    /// ��ն���
    void Clear();

    /// URL�Ƿ���Ч
    inline bool IsValid() const
    {
        return (IsSchemeValid(m_scheme, m_valid_schemes) && !m_host.empty() && !m_path.empty());
    }

    /// URL�Ƿ�Ϊ��̬ҳ
    inline bool IsDynamicPage() const
    {
        return m_has_query;
    }

    /// URL�Ƿ�Ϊ��ҳ
    inline bool IsHomePage() const
    {
        return (!IsDynamicPage() && (m_path == "/"));
    }

    /// ��ȡ�淶�����URL
    std::string GetNormalizedUrl(int normalize_flag = NORMALIZE_FOR_DOCID) const;

    /// ��ȡ��תURL��ģʽΪ��<reversed domain>[#port]<"scheme"><relative path>
    bool GetReversedUrl(void * reversed_url, int & reversed_url_len,
                        int normalize_flag = NORMALIZE_FOR_DOCID) const;

    std::string GetReversedUrl(int normalize_flag = NORMALIZE_FOR_DOCID) const;

    /// ��ȡ����·��
    inline const std::string & GetAbsolutePath() const
    {
        return m_absolute_path;
    }

    inline const char * GetAbsolutePathPtr() const
    {
        return m_absolute_path.c_str();
    }

    /// ��ȡ���·��
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

    /// ��ȡ���·��
    inline std::string GetPortalPath() const
    {
        return (m_portal_path_end_pos == std::string::npos) ?
               std::string("") : m_absolute_path.substr(0, m_portal_path_end_pos);
    }

    /// ��ȡƬ��·��
    std::string GetSegmentPath() const;

    /// ��ȡ��תƬ��·��
    std::string GetReversedSegmentPath() const;

    /// ��ȡ��ҳURL
    inline std::string GetHomePageURL() const
    {
        return (m_relative_path_start_pos == std::string::npos) ?
               std::string("") : m_absolute_path.substr(0, m_relative_path_start_pos + 1);
    }

    /// ��ȡЭ��
    inline EScheme GetScheme() const
    {
        return m_scheme;
    }

    /// ��ȡЭ���ַ���
    std::string GetSchemeString() const;

    /// ��ȡ����
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

    /// ��ȡ���˿ڵ�����
    std::string GetHostPort() const;

    DEPRECATED_BY(GetHostPort)
    inline std::string GetDomainWithPort() const
    {
        return GetHostPort();
    }

    /// ��ȡ��ת����
    inline std::string GetReversedHost() const
    {
        return ReverseDomain(m_host);
    }

    DEPRECATED_BY(GetReversedHost)
    inline std::string GetReversedDomain() const
    {
        return GetReversedHost();
    }

    /// ��ȡ���˿ڵķ�ת����
    inline std::string GetReversedHostPort() const
    {
        return ReverseDomain(GetHostPort());
    }

    DEPRECATED_BY(GetReversedHostPort)
    inline std::string GetReversedDomainWithPort() const
    {
        return GetReversedHostPort();
    }

    /// ��ȡ�˿�
    inline uint16_t GetPort() const
    {
        return m_port;
    }

    /// ��ȡ·��
    inline const std::string & GetPath() const
    {
        return m_path;
    }

    inline const char * GetPathPtr() const
    {
        return m_path.c_str();
    }

    /// ��ȡ��ѯ
    void GetQuerys(std::map<std::string, std::string> & querys) const;

    /// ��ȡĿ¼
    std::string GetDirectory() const;

    /// ��ȡ���в㼶Ŀ¼
    void GetDirectories(std::vector<std::string> & directories) const;

    /// ��ȡ��Դ��
    inline std::string GetResource() const
    {
        return (m_resource_pos == std::string::npos) ?
               std::string("") : m_path.substr(m_resource_pos);
    }

    inline const char * GetResourcePtr() const
    {
        return (m_resource_pos == std::string::npos) ? NULL : (m_path.c_str() + m_resource_pos);
    }

    /// ��ȡ��Դ��׺��
    std::string GetResourceSuffix() const;

    const char * GetResourceSuffixPtr() const;

    /// ��ȡURL��·����ȣ�Ŀ¼�㼶��
    inline unsigned int GetPathDepth() const
    {
        return m_directory_pos_list.size();
    }

public:
    /// ��URL���б��룬Ĭ�ϲ����뱣����
    /// ����Ҫ��URL��Ϊ�ļ�������ѡ����뱣����
    /// ����Ҫ������ʾ�����ַ�����ѡ�񲻱�����չ�ַ���
    enum
    {
        E_ENCODE_RESERVED_CHAR = 0x01,          ///< ���뱣����
        E_NOT_ENCODE_EXTENDED_CHAR_SET = 0x02   ///< ��������չ�ַ���(ASCIIֵ����127���ַ�)
    };
    static std::string EncodeUrl(const std::string & url, int flag = 0);

    /// ��URL���н��룬Ĭ�ϲ����뱣���ֺͰٷֺ��ַ�
    enum
    {
        E_DECODE_RESERVED_CHAR = 0x10,          ///< ���뱣����
        E_DECODE_PERCENT_SIGN_CHAR = 0x20       ///< ����ٷֺ��ַ�
    };
    static std::string DecodeUrl(const std::string & url, int flag = 0);

    /// ��ת������֧�ִ��˿ڵ�������ת��domain[:port] <==> reversed domain[#port]
    /// @param  domain Ҫ��ת������
    /// @return ��ת�������
    static std::string ReverseDomain(const std::string & domain);

    /// ��ת������֧�ִ��˿ڵ�������ת��domain[:port] <==> reversed domain[#port]
    /// @param  src     Ҫ��ת�������ַ���
    /// @param  src_len Ҫ��ת�������ַ�������
    /// @param  dest    ������ŷ�ת���������ַ���
    /// @return ָ��ת�������ַ�������ָ��
    static char * ReverseDomain(const char * src, const int src_len, char * dest);

public:
    /// ��ȡURL��ϣ
    uint64_t GetUrlHash() const;

    /// ��ȡ������ϣ
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

    /// ��ȡ���·����ϣ
    uint64_t GetPortalPathHash() const;

    /// ��ȡ��תƬ��·����ϣ
    uint64_t GetReversedSegmentPathHash() const;

private:
    /// Ԥ����
    bool Preprocess(const char * in_url, const int in_url_len,
                    std::string * out_url, bool decode_percent_encoded) const;

    /// ����URL
    bool Parse(const char * url, const int url_len, EScheme default_scheme);

    /// ����Э��
    bool ParseScheme(const char *& start, const char * end,
                     EScheme default_scheme, bool & has_scheme);

    /// ��������
    bool ParseHost(const char *& start, const char * end);

    /// ����·��
    bool ParsePath(const char *& start, const char * end, bool has_scheme);

    /// ������ѯ
    bool ParseQuery(const char *& start, const char * end);

    /// ������ѯ�ַ���
    static void ParseQueryString(const std::string & query,
                                 std::vector<QueryPos> * query_pos_list);

    /// �����ѯ
    static void SortQuery(const std::string & query, std::vector<QueryPos> * query_pos_list);

    /// ����URL
    void Recompose(int normalize_flag);

    /// �����ѯ
    static void RecomposeQuery(const std::string & query,
                               const std::vector<QueryPos> & query_pos_list,
                               std::string * recomposed_query);

    /// ƴ��·��
    void JoinPath(const std::string & path);

    /// ����·��
    static void RemoveDotSegments(std::string * path);

    /// ����������Ϣ
    inline void CopyHostInfo(const Url & url)
    {
        m_host = url.m_host;
        m_port = url.m_port;
    }

    /// ����·����Ϣ
    inline void CopyPathInfo(const Url & url)
    {
        m_path = url.m_path;
    }

    /// ���Ʋ�ѯ��Ϣ
    inline void CopyQueryInfo(const Url & url)
    {
        m_query = url.m_query;
        m_query_pos_list = url.m_query_pos_list;
        m_has_query = url.m_has_query;
    }

    /// ����Э��Ͷ˿�
    void SetSchemeAndPort(EScheme scheme);

    /// ����Ĭ��·��
    inline void SetDefaultPath()
    {
        m_path = "/";
    }

    /// �Ƿ�Ϊ��ЧЭ��
    static inline bool IsSchemeValid(EScheme scheme, int valid_schemes)
    {
        if ((scheme != E_NO_PROTOCOL) && (scheme != E_UNKNOWN_PROTOCOL))
        {
            return ((1 << scheme) & valid_schemes);
        }

        return false;
    }

    /// �Ƿ�����ʽ�˿�
    bool HasExplicitPort() const;

    /// �Ƿ�ΪĬ��ҳ
    static bool IsDefaultPage(const char * resource);

    /// �ж��ַ����Ƿ�Ϊ�ٷֺű���
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

    /// ȥ����β���˵Ŀո�
    static int TrimSpace(const char *& start, const char *& end);

private:
    /// <scheme>://<authority>/<path>;<params>?<query>#<fragment>
    EScheme m_scheme;                   ///< Э��
    std::string m_host;                 ///< ����
    uint16_t m_port;                    ///< �˿�
    std::string m_path;                 ///< ·��
    std::string m_query;                ///< ��ѯ
    bool m_has_query;                   ///< �Ƿ��в�ѯ

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
