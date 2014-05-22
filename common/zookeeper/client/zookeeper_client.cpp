// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <vector>
#include "common/base/scoped_ptr.h"
#include "common/base/string/algorithm.h"
#include "common/crypto/hash/sha1.h"
#include "common/encoding/base64.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "common/zookeeper/client/zookeeper_node.h"
#include "common/zookeeper/client/zookeeper_node_mock.h"
#include "common/zookeeper/client/zookeeper_session.h"

namespace common {

const std::string kMockZookeeperCluster("mock");
const std::string kMockZookeeperErrorPath("ZK_");

ZookeeperClient::ZookeeperClient(const Options& options, Closure<void, std::string, int>* handler)
{
    m_session_event_handler = handler;
    m_options.readonly = options.readonly;
    m_options.timeout = options.timeout;
    m_options.credential = options.credential;
    if (FLAGS_zk_test_env_enable && m_options.readonly) {
        CHECK(!FLAGS_zk_test_proxy_host.empty()) << "Use taas test environment,"
            " but not set test proxy host address.";
    }
}

ZookeeperClient::~ZookeeperClient()
{
    Close();
    if (m_session_event_handler != NULL) {
        delete m_session_event_handler;
        m_session_event_handler = NULL;
    }
}

void ZookeeperClient::Close()
{
    std::map<std::string, ZookeeperSession*> sessions;
    { // scoped lock.
        MutexLocker locker(m_mutex);
        sessions.swap(m_sessions);
    }

    std::map<std::string, ZookeeperSession*>::iterator iter;
    for (iter = sessions.begin(); iter != sessions.end(); ++iter) {
        iter->second->Close();
        delete iter->second;
    }
}

void ZookeeperClient::ChangeIdentity(const std::string& credential)
{
    MutexLocker locker(m_mutex);
    m_options.credential = credential;
    std::map<std::string, ZookeeperSession*>::iterator iter;
    for (iter = m_sessions.begin(); iter != m_sessions.end(); ++iter) {
        iter->second->ChangeIdentity(credential);
    }
}

bool ZookeeperClient::GetProxyName(const std::string& cluster_name, std::string* proxy) const
{
    if (cluster_name.size() < 2) {
        return false;
    }
    if (FLAGS_zk_test_env_enable) {
        *proxy = FLAGS_zk_test_proxy_host;
        return true;
    }
    *proxy = cluster_name.substr(0, 2);
    *proxy += ".proxy.zk.oa.com:2181";
    return true;
}

bool ZookeeperClient::GetClusterName(const std::string& path, std::string* cluster_name) const
{
    std::vector<std::string> sections;
    SplitString(path, "/", &sections);
    if (sections.size() < 2) {
        return false;
    }
    // section[1] is the cluster name
    *cluster_name = sections[1] + ".zk.oa.com:2181";
    return true;
}

bool ZookeeperClient::GetMockRetCode(const std::string& path, int* ret_code)
{
    std::vector<std::string> sections;
    SplitString(path, "/", &sections);
    if (sections.size() < 3) {
        return false;
    }
    if (!StringStartsWith(sections[2], kMockZookeeperErrorPath)) {
        return false;
    }
    CHECK(StringToZookeeperErrorCode(sections[2].c_str(), ret_code))
        << sections[2] << " is not an error code of zookeeper client.";
    return true;
}

ZookeeperNode* ZookeeperClient::Open(const std::string& path, int* error_code)
{
    VLOG(1) << "Try zk client open, with path : " << path;
    int error_placeholder = 0;
    if (error_code == NULL) {
        error_code = &error_placeholder;
    }
    *error_code = ZK_OK;

    // Get zookeeper cluster name from node path.
    std::string cluster_name;
    if (!GetClusterName(path, &cluster_name)) {
        *error_code = ZK_INVALIDPATH;
        return NULL;
    }

    if (StringStartsWith(cluster_name, kMockZookeeperCluster)) {
        int ret_code = ZK_OK;
        // 1. /zk/mock/xxx; zk node mock, return ZK_OK
        // 2. /zk/mock/RET_CODE; zk node mock, return RET_CODE
        GetMockRetCode(path, &ret_code);
        return new ZookeeperNodeMock(ret_code);
    }

    if (m_options.readonly) {
        if (!GetProxyName(cluster_name, &cluster_name)) {
            *error_code = ZK_INVALIDPATH;
            return NULL;
        }
    }
    ZookeeperSession* session = FindOrMakeSession(cluster_name);
    if (session == NULL) {
        *error_code = ZK_DISCONNECTED;
        return NULL;
    }
    return session->Open(path, error_code);
}

namespace { // anonymous namespace

bool GetClusterNameAndPathFromFullPath(const std::string& full_path,
                                       std::string* cluster_name,
                                       std::string* zk_path)
{
    std::string::size_type pos = full_path.find("/");
    if (pos == std::string::npos) {
        return false;
    }
    *cluster_name = full_path.substr(0, pos);
    *zk_path = full_path.substr(pos);
    return true;
}

} // anonymous namespace

ZookeeperNode* ZookeeperClient::OpenWithFullPath(const std::string& path, int* error_code)
{
    int error_placeholder = 0;
    if (error_code == NULL) {
        error_code = &error_placeholder;
    }
    *error_code = ZK_OK;

    // Get zookeeper cluster name from node path.
    std::string cluster_name;
    std::string zk_path;
    if (!GetClusterNameAndPathFromFullPath(path, &cluster_name, &zk_path)) {
        *error_code = ZK_INVALIDPATH;
        return NULL;
    }
    ZookeeperSession* session = FindOrMakeSession(cluster_name);
    if (session == NULL) {
        *error_code = ZK_DISCONNECTED;
        return NULL;
    }
    return session->Open(zk_path, error_code);
}

ZookeeperNode* ZookeeperClient::Create(const std::string& path,
                                       const std::string& data,
                                       int  flags,
                                       int* error_code)
{
    int error_placeholder = 0;
    if (error_code == NULL) {
        error_code = &error_placeholder;
    }
    *error_code = ZK_OK;

    if (m_options.readonly) {
        *error_code = ZK_READONLY;
        return NULL;
    }

    // Get zookeeper cluster name from node path.
    std::string cluster_name;
    if (!GetClusterName(path, &cluster_name)) {
        *error_code = ZK_INVALIDPATH;
        return NULL;
    }

    if (StringStartsWith(cluster_name, kMockZookeeperCluster)) {
        int ret_code = ZK_OK;
        // 1. /zk/mock/xxx; zk node mock, return ZK_OK
        // 2. /zk/mock/RET_CODE; zk node mock, return RET_CODE
        GetMockRetCode(path, &ret_code);
        return new ZookeeperNodeMock(ret_code);
    }

    ZookeeperSession* session = FindOrMakeSession(cluster_name);
    if (session == NULL) {
        *error_code = ZK_DISCONNECTED;
        return NULL;
    }
    scoped_array<char> buffer(new char[ZookeeperNode::kMaxBufferLen]);
    // TODO(hsiaokangliu): Use ZK_AUTH_IDS instead of ZK_OPEN_ACL_UNSAFE
    *error_code = session->Create(
            path.c_str(), data.data(), data.size(),
            &ZOO_OPEN_ACL_UNSAFE, flags, buffer.get(), ZookeeperNode::kMaxBufferLen);
    if (*error_code == ZK_OK) {
        std::string real_path = buffer.get();
        return session->Open(real_path, error_code);
    } else if (*error_code == ZK_NODEEXISTS) {
        return session->Open(path, error_code);
    }
    // failed to create node.
    return NULL;
}

ZookeeperSession* ZookeeperClient::FindOrMakeSession(const std::string& cluster)
{
    ZookeeperSession* session = NULL;
    MutexLocker locker(m_mutex);
    std::map<std::string, ZookeeperSession*>::iterator iter;
    iter = m_sessions.find(cluster);
    if (iter != m_sessions.end()) {
        VLOG(1) << "Try connect to zk cluster : " << cluster << ", use exist session";
        session = iter->second;
        int error_code = session->Connect();
        if (error_code != ZK_OK) {
            LOG(WARNING) << "Failed to init to zookeeper cluster: "
                << cluster << ", error text: "
                << ZookeeperErrorString(static_cast<ZookeeperErrorCode>(error_code))
                << "(" << error_code << ")";
            session = NULL;
        }
    } else {
        VLOG(1) << "Try connect to zk cluster : " << cluster << ", with new zk session";
        session = new ZookeeperSession(cluster, m_options, m_session_event_handler);
        int error_code = session->Connect();
        if (error_code == ZK_OK) {
            // new session established.
            m_sessions[cluster] = session;
        } else {
            LOG(WARNING) << "Failed to init to zookeeper cluster: "
                << cluster << ", use new zk-session, error text: "
                << ZookeeperErrorString(static_cast<ZookeeperErrorCode>(error_code))
                << "(" << error_code << ")";
            delete session;
            session = NULL;
        }
    }
    return session;
}

bool ZookeeperClient::Encrypt(const std::string& id_passwd, std::string* encrypted_passwd)
{
    encrypted_passwd->clear();
    std::string::size_type pos = id_passwd.find(":");
    if (pos == std::string::npos) {
        return false;
    }
    char buffer[SHA1::kDigestLength + 1];
    SHA1::Digest(id_passwd.data(), id_passwd.size(), buffer);
    buffer[SHA1::kDigestLength] = '\0';
    bool ret = Base64::Encode(buffer, encrypted_passwd);
    *encrypted_passwd = id_passwd.substr(0, pos + 1) + *encrypted_passwd;
    return ret;
}

} // namespace common
