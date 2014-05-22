// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <fstream>
#include <vector>

#include "common/base/compatible/stdio.h"
#include "common/base/scoped_ptr.h"
#include "common/base/string/algorithm.h"
#include "common/base/string/string_number.h"
#include "common/system/concurrency/this_thread.h"
#include "common/system/io/directory.h"
#include "common/system/io/file.h"
#include "common/system/io/textfile.h"
#include "common/system/net/domain_resolver.h"
#include "common/system/process.h"
#include "common/system/time/time_utils.h"
#include "common/zookeeper/client/zookeeper_node.h"
#include "common/zookeeper/client/zookeeper_session.h"

namespace common {

DEFINE_bool(enable_hosts_cache, true, "if enable zk hosts cache");
DEFINE_bool(enable_test_hosts, false, "if use zk test hosts address");
DEFINE_string(root_zk, "10.168.147.208:2181,10.168.154.110:2181,10.168.147.16:2181", "root zk ip");

const std::string kRootZookeeperCluster = "tf.zk.oa.com:2181";
const std::string kHostsPath = "/zk/tf/router/zk_hosts";
const std::string kTestHostsPath = "/zk/tf/router/test_hosts";

// update domain ip list cache file by each 5 minutes.
const int kCacheUpdateTime = 60 * 5;

ZookeeperSession::ZookeeperSession(const std::string& cluster,
                                   const ZookeeperClient::Options& options,
                                   Closure<void, std::string, int>* handler) :
        m_cluster_name(cluster),
        m_status(Status_Init),
        m_handle(NULL),
        m_client_id(NULL),
        m_readonly(options.readonly),
        m_timeout(options.timeout),
        m_credential(options.credential),
        m_session_event_handler(handler)
{
    m_callback = NewPermanentClosure(this, &ZookeeperSession::OnSessionEvent);
}

ZookeeperSession::~ZookeeperSession()
{
    Close();
    delete m_callback;
    m_callback = NULL;
}

void ZookeeperSession::Close()
{
    zhandle_t* handle = NULL;
    {
        MutexLocker locker(m_mutex);
        m_status = Status_Init;
        handle = m_handle;
        m_handle = NULL;
        m_client_id = NULL;
        m_id.clear();
    }
    CloseHandle(handle);
    m_watcher_manager.ProcessResidentWatcher(ZK_SESSION_CLOSED_EVENT);
}

void ZookeeperSession::CloseHandle(zhandle_t* handle)
{
    if (handle != NULL) {
        zookeeper_close(handle);
    }
}

ZookeeperNode* ZookeeperSession::Open(const std::string& path, int* error_code)
{
    int error_placeholder = 0;
    if (error_code == NULL) {
        error_code = &error_placeholder;
    }
    int status = GetStatus();
    if (status != Status_Connected && status != Status_Connecting) {
        *error_code = ZK_DISCONNECTED;
        return NULL;
    }
    ZookeeperNode* node = new ZookeeperNode();
    if (node == NULL) {
        *error_code = ENOMEM;
        LOG(ERROR) << "Failed to allocate memory.";
        return NULL;
    }
    node->SetSession(this);
    node->SetPath(path);
    return node;
}

namespace { // anonymous namespace
void SessionWatcher(zhandle_t* handle, int event_type, int state, const char* path, void* context)
{
    Closure<void, int>* callback = static_cast<Closure<void, int>*>(context);
    // Only session events will be processed.
    if (event_type == ZOO_SESSION_EVENT) {
        if (state == ZOO_EXPIRED_SESSION_STATE) {
            callback->Run(ZK_SESSION_EXPIRED_EVENT);
        } else if (state == ZOO_AUTH_FAILED_STATE) {
            callback->Run(ZK_SESSION_AUTH_FAILED_EVENT);
        } else if (state == ZOO_CONNECTING_STATE) {
            callback->Run(ZK_SESSION_CONNECTING_EVENT);
        } else if (state == ZOO_CONNECTED_STATE) {
            callback->Run(ZK_SESSION_CONNECTED_EVENT);
        }
    }
}
} // anonymous namespace

int ZookeeperSession::Connect()
{
    // If this session has expired, close it first, then connect.
    if (GetStatus() == Status_Expired) {
        Close();
    }
    {
        m_connect_event.Reset();
        MutexLocker lock(m_mutex);
        if (m_status == Status_Connecting || m_status == Status_Connected) {
            // Already connecting or connected.
            return ZK_OK;
        }
        // Random select a zookeeper server
        zoo_deterministic_conn_order(0);
        m_status = Status_Connecting;

        std::string cluster_ips;
        int64_t t1 = TimeUtils::Milliseconds();
        ResolveServerAddress(m_cluster_name, &cluster_ips);
        VLOG(1) << "Dns time cost: " << TimeUtils::Milliseconds() - t1 << " ms";
        if (cluster_ips.empty()) {
            m_status = Status_Init;
            return ZK_DNSFAILURE;
        }
        m_handle = zookeeper_init(
            cluster_ips.c_str(), SessionWatcher, m_timeout, NULL, m_callback, 0);

        if (m_handle == NULL) {
            m_status = Status_Init;
            return errno;
        }
    }
    // Wait until this sync event is set.
    bool ret = m_connect_event.Wait(m_timeout);
    if (!ret) {
        Close();  // Close opened handle.
        return ZK_OPERATIONTIMEOUT;
    }

    int state = 0;
    clientid_t* client_id = NULL;
    {
        MutexLocker lock(m_mutex);
        if (m_handle == NULL) {
            m_status = Status_Init;
            return ZK_INVALIDSTATE;
        }
        // Get status and client id.
        state = zoo_state(m_handle);
        client_id = const_cast<clientid_t*>(zoo_client_id(m_handle));
    }
    if (state != ZOO_CONNECTED_STATE) {
        Close();  // Close opened handle.
        return ZK_INVALIDSTATE;
    }
    if (client_id == NULL) {
        Close();  // Close opened handle.
        return ZK_INVALIDCLIENTID;
    }
    {
        MutexLocker locker(m_mutex);
        m_client_id = client_id;
        UpdateId();
        m_status = Status_Connected;
    }
    return ZK_OK;
}

bool ZookeeperSession::IsRecoverableError(int error_code)
{
    if (error_code == ZK_OPERATIONTIMEOUT ||
        error_code == ZK_CONNECTIONLOSS ||
        error_code == ZK_SESSIONMOVED) {
        return true;
    }
    return false;
}

bool ZookeeperSession::UpdateCacheFile()
{
    std::string cache_file = kDomainCacheFile;
    if (FLAGS_enable_test_hosts) {
        cache_file = kDomainTestCacheFile;
    }
    if (io::file::Exists(cache_file)) {
        time_t t1 = time(NULL);
        time_t t2 = io::file::GetLastModifyTime(cache_file);
        if (t1 - t2 < kCacheUpdateTime) {
            return true;
        }
    }
    // TODO(rabbitliu) Use proxy instead of real root zk server.
    std::string cluster_ips;
    ResolveServerAddress(kRootZookeeperCluster, &cluster_ips);
    if (cluster_ips.empty()) {
        return false;
    }
    ZookeeperSession session(cluster_ips, ZookeeperClient::Options(), NULL);
    int error_code = session.Connect();
    if (error_code != ZK_OK) {
        return false;
    }
    scoped_ptr<ZookeeperNode> node;
    if (FLAGS_enable_test_hosts) {
        node.reset(session.Open(kTestHostsPath, NULL));
    } else {
        node.reset(session.Open(kHostsPath, NULL));
    }
    if (node == NULL) {
        return false;
    }
    std::string contents;
    if (node->GetContent(&contents) != ZK_OK) {
        return false;
    }
    if (!io::directory::Exists(kCacheDir)) {
        if (!io::directory::Create(kCacheDir)) {
            return false;
        }
        if (chmod(kCacheDir.c_str(), 0777) != 0) {
            LOG(ERROR) << "Can not change access model on path: " << kCacheDir
                << ", error text: " << strerror(errno);
            return false;
        }
    }
    std::string temp_file = cache_file + "_" + IntegerToString(getpid());
    std::ofstream file_stream(temp_file.c_str());
    if (!file_stream) {
        return false;
    }
    file_stream << contents << "\n";
    file_stream.close();
    if (!io::file::Rename(temp_file, cache_file)) {
        return false;
    }
    if (chmod(cache_file.c_str(), 0666) != 0) {
        LOG(ERROR) << "Can not change access model on path: " << cache_file
                   << ", error text: " << strerror(errno);
        return false;
    }
    return true;
}

bool ZookeeperSession::ResolveIpFromCache(const std::string& cluster, std::string* cluster_ips)
{
    if (!UpdateCacheFile()) {
        // update cache file failure, just go on and use old cache file to get domain ip.
        LOG(WARNING) << "update cache file failure.";
    }
    std::string cache_file = kDomainCacheFile;
    if (FLAGS_enable_test_hosts) {
        cache_file = kDomainTestCacheFile;
    }
    std::vector<std::string> domain_ip_vector;
    if (!io::textfile::ReadLines(cache_file, &domain_ip_vector)) {
        LOG(WARNING) << "Can not get ip from cache file, read cache file failure.";
        return false;
    }
    std::vector<std::string>::iterator it = domain_ip_vector.begin();
    for (; it != domain_ip_vector.end(); ++it) {
        if (StringStartsWith(*it, cluster)) {
            size_t index = it->find("=");
            if (index == std::string::npos) {
                LOG(WARNING) << "Can not get ip from cache file, invalid format line: " << *it;
                return false;
            }
            *cluster_ips = it->substr(index + 1);
            return true;
        }
    }
    LOG(WARNING) << "Can not get ip from cache file, cluster name not exists in cache: "
                 << cluster;
    return false;
}

bool ZookeeperSession::ResolveServerAddress(const std::string& cluster, std::string* ips)
{
    std::string::size_type pos = cluster.find(",");
    if (pos != std::string::npos) {
        // cluster name seperated by comma, it must be ip list.
        ips->assign(cluster);
        return true;
    }
    pos = cluster.find(":");
    if (pos == std::string::npos) {
        // not a valid ip:port address
        return false;
    }
    ips->clear();
    int error_code = 0;
    std::vector<IPAddress> addresses;
    std::string cluster_name = cluster.substr(0, pos);
    std::string cluster_port = cluster.substr(pos + 1);
    if (DomainResolver::ResolveIpAddress(cluster_name, &addresses, &error_code)) {
        // use dns server to resolve address first
        for (size_t i = 0; i < addresses.size(); ++i) {
            if (i > 0) {
                ips->append(",");
            }
            ips->append(addresses[i].ToString() + ":" + cluster_port);
        }
        return true;
    } else {
        // dns lookup failure.
        if (cluster == kRootZookeeperCluster) {
            // for root zk, we can't get it's ip from any other place, so assign a default value.
            ips->assign(FLAGS_root_zk);
            return true;
        } else if (FLAGS_enable_hosts_cache && ResolveIpFromCache(cluster, ips)) {
            // for other zk connections, if cache and root zk enabled, get address from cache.
            return true;
        }
    }
    return false;
}

int ZookeeperSession::ChangeIdentity(const std::string& auth_key)
{
    MutexLocker locker(m_mutex);
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    // "digest" password based authentication
    // Sync function, don't care callback.
    return zoo_add_auth(m_handle, "digest", auth_key.data(), auth_key.length(), NULL, NULL);
}

int ZookeeperSession::Create(const char* path,
        const char* value,
        int value_length,
        ACL_vector* acl,
        int flags,
        char* buffer,
        int buffer_length)
{
    MutexLocker locker(m_mutex);
    if (m_readonly) {
        return ZK_READONLY;
    }
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    int ret = 0;
    int retry_num = 0;
    while (retry_num <= kMaxRetryNumber) {
        retry_num++;
        ret = zoo_create(
                m_handle,               // zookeeper handle
                path,                   // node name
                value,                  // node value
                value_length,           // node value length
                acl,                    // ACL
                flags,                  // node type
                buffer,
                buffer_length
                );
        if (ret == ZK_OK || !IsRecoverableError(ret)) {
            // no need to retry.
            return ret;
        }
    }
    return ret;
}

int ZookeeperSession::Get(const char* path, int watch, char* buffer, int* length, Stat* stat)
{
    MutexLocker locker(m_mutex);
    if (watch != 0 && m_readonly) {
        return ZK_READONLY;
    }
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    int ret = 0;
    int retry_num = 0;
    VLOG(1) << "Try get content, path : " << path;
    while (retry_num <= kMaxRetryNumber) {
        retry_num++;
        ret = zoo_get(m_handle, path, watch, buffer, length, stat);
        if (ret == ZK_OK || !IsRecoverableError(ret)) {
            // no need to retry.
            return ret;
        }

        if (retry_num <= kMaxRetryNumber) {
            ThisThread::Sleep(500); // 500 milliseconds
            VLOG(1) << "zoo_get not Ok, try again. tried times : " << retry_num;
        }
    }
    return ret;
}

int ZookeeperSession::Set(const char* path, const char* buffer, int length, int version)
{
    MutexLocker locker(m_mutex);
    if (m_readonly) {
        return ZK_READONLY;
    }
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    int ret = 0;
    int retry_num = 0;
    while (retry_num <= kMaxRetryNumber) {
        retry_num++;
        ret = zoo_set(m_handle, path, buffer, length, version);
        if (ret == ZK_OK || !IsRecoverableError(ret)) {
            // no need to retry.
            return ret;
        }
    }
    return ret;
}

int ZookeeperSession::Delete(const char* path, int version)
{
    MutexLocker locker(m_mutex);
    if (m_readonly) {
        return ZK_READONLY;
    }
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    int ret = 0;
    int retry_num = 0;
    while (retry_num <= kMaxRetryNumber) {
        retry_num++;
        ret = zoo_delete(m_handle, path, version);
        if (ret == ZK_OK || !IsRecoverableError(ret)) {
            // no need to retry.
            return ret;
        }
    }
    return ret;
}

int ZookeeperSession::Exists(const char* path, int watch, Stat* stats)
{
    MutexLocker locker(m_mutex);
    if (watch != 0 && m_readonly) {
        return ZK_READONLY;
    }
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    int ret = 0;
    int retry_num = 0;
    while (retry_num <= kMaxRetryNumber) {
        retry_num++;
        ret = zoo_exists(m_handle, path, watch, stats);
        if (ret == ZK_OK || !IsRecoverableError(ret)) {
            // no need to retry.
            return ret;
        }
    }
    return ret;
}

int ZookeeperSession::GetChildren(const char* path, int watch, String_vector* children)
{
    MutexLocker locker(m_mutex);
    if (watch != 0 && m_readonly) {
        return ZK_READONLY;
    }
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    int ret = 0;
    int retry_num = 0;
    while (retry_num <= kMaxRetryNumber) {
        retry_num++;
        ret = zoo_get_children(m_handle, path, watch, children);
        if (ret == ZK_OK || !IsRecoverableError(ret)) {
            // no need to retry.
            return ret;
        }
    }
    return ret;
}

int ZookeeperSession::GetAcl(const char* path, struct ACL_vector* acl, struct Stat* stat)
{
    MutexLocker locker(m_mutex);
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    return zoo_get_acl(m_handle, path, acl, stat);
}

int ZookeeperSession::SetAcl(const char* path, const struct ACL_vector*acl, int version)
{
    MutexLocker locker(m_mutex);
    if (m_readonly) {
        return ZK_READONLY;
    }
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    return zoo_set_acl(m_handle, path, version, acl);
}

int ZookeeperSession::WatcherExists(const char* path,
        watcher_fn watcher,
        void* context,
        Stat* stat)
{
    MutexLocker locker(m_mutex);
    if (m_readonly) {
        return ZK_READONLY;
    }
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    int ret = 0;
    int retry_num = 0;
    while (retry_num <= kMaxRetryNumber) {
        retry_num++;
        ret = zoo_wexists(m_handle, path, watcher, context, stat);
        if (ret == ZK_OK || !IsRecoverableError(ret)) {
            // no need to retry.
            return ret;
        }
    }
    return ret;
}

int ZookeeperSession::WatcherGet(const char* path,
        watcher_fn watcher,
        void* context,
        char* buffer,
        int* buffer_length,
        Stat* stat)
{
    MutexLocker locker(m_mutex);
    if (m_readonly) {
        return ZK_READONLY;
    }
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    int ret = 0;
    int retry_num = 0;
    while (retry_num <= kMaxRetryNumber) {
        retry_num++;
        ret = zoo_wget(m_handle, path, watcher, context, buffer, buffer_length, stat);
        if (ret == ZK_OK || !IsRecoverableError(ret)) {
            // no need to retry.
            return ret;
        }
    }
    return ret;
}

int ZookeeperSession::WatcherGetChildren(const char* path,
        watcher_fn watcher,
        void* context,
        String_vector* strings)
{
    MutexLocker locker(m_mutex);
    if (m_readonly) {
        return ZK_READONLY;
    }
    if (m_handle == NULL) {
        return ZK_INVALIDSTATE;
    }
    int ret = 0;
    int retry_num = 0;
    while (retry_num <= kMaxRetryNumber) {
        retry_num++;
        ret = zoo_wget_children(m_handle, path, watcher, context, strings);
        if (ret == ZK_OK || !IsRecoverableError(ret)) {
            // no need to retry.
            return ret;
        }
    }
    return ret;
}

void ZookeeperSession::UpdateId()
{
    if (m_client_id == NULL) {
        return;
    }
    char buffer[30];
#if defined(__x86_64__)
    snprintf(buffer, sizeof(buffer), "%#lx",  m_client_id->client_id);
#else
    snprintf(buffer, sizeof(buffer), "%#llx", m_client_id->client_id);
#endif
    m_id.assign(buffer, strlen(buffer));
}

void ZookeeperSession::OnSessionEvent(int event)
{
    if (GetStatus() == Status_Connecting) {
        m_connect_event.Set();
    }
    if (event == ZK_SESSION_EXPIRED_EVENT) {
        LOG(ERROR) << "session expired: " << m_cluster_name;
        // set session status to expired.
        MutexLocker locker(m_mutex);
        m_status = Status_Expired;
    }
    // notify session events.
    if (m_session_event_handler) {
        m_session_event_handler->Run(m_cluster_name, event);
    }
    if (event == ZK_SESSION_EXPIRED_EVENT) {
        // auto reconnect when session expired.
        Connect();
    }
}

} // namespace common
