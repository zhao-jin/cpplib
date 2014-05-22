// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_SESSION_H
#define COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_SESSION_H

#include <map>
#include <string>
#include "common/system/concurrency/event.h"
#include "common/system/concurrency/mutex.h"
#include "common/zookeeper/client/watcher_manager.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "thirdparty/gtest/gtest_prod.h"
#include "thirdparty/zookeeper/zookeeper.h"

namespace common {

DECLARE_bool(enable_hosts_cache);
DECLARE_bool(enable_test_hosts);

const std::string kCacheDir = "/tmp/zookeeper";
const std::string kDomainCacheFile = "/tmp/zookeeper/zk_hosts";
const std::string kDomainTestCacheFile = "/tmp/zookeeper/test_hosts";

class ZookeeperNode;

class ZookeeperSession
{
public:
    static const int kMaxRetryNumber = 3;

    enum SessionStatus
    {
        Status_Init = 0,
        Status_Connecting = 1,
        Status_Connected = 2,
        Status_Expired = 3,
    };

    /// @param cluster cluster name to connect to.
    /// @param options ZookeeperClient defined options.
    /// @param handler Session event handler.
    ZookeeperSession(const std::string& cluster,
                     const ZookeeperClient::Options& options,
                     Closure<void, std::string, int>* handler);
    virtual ~ZookeeperSession();

    /// @brief Open a zookeeper node, no matter this node exists.
    /// @param path node path, e.g. /zk/tjd1/xcube
    ZookeeperNode* Open(const std::string& path, int* error_code);

    /// @brief Connect to a zookeeper server synchronously.
    /// @retval error_code
    int Connect();

    /// @brief Close this session.
    void Close();

    /// @brief Set connection authorization. wrapper of zoo_add_auth.
    int ChangeIdentity(const std::string& credential);

    /// @brief Create a zookeeper node. wrapper of zoo_create
    int Create(const char* path, const char* value, int value_length,
            ACL_vector* acl, int flags, char* buffer, int buffer_length);
    /// @brief Get a zookeeper node value. wrapper of zoo_get.
    int Get(const char* path, int watch, char* buffer, int* length, Stat* stat);
    /// @brief Set a zookeeper node value. wrapper of zoo_set.
    int Set(const char* path, const char* buffer, int length, int version = -1);
    /// @brief Delete a zookeeper node. wrapper of zoo_delete.
    int Delete(const char* path, int version = -1);
    /// @brief Check if node exists. wrapper of zoo_exists.
    int Exists(const char* path, int watch, Stat* stats);
    /// @brief Get node children. wrapper of zoo_get_children.
    int GetChildren(const char* path, int watch, String_vector* children);
    int GetAcl(const char* path, struct ACL_vector* acl, struct Stat* stat);
    int SetAcl(const char* path, const struct ACL_vector* acl, int version = -1);

    /// @brief wrapper of zoo_wexists.
    int WatcherExists(const char* path, watcher_fn watcher, void* context, Stat* stat);
    /// @brief wrapper of zoo_wget.
    int WatcherGet(const char* path,
            watcher_fn watcher,
            void* context,
            char* buffer,
            int* buffer_length,
            Stat* stat);
    /// @brief wrapper of zoo_wget_children.
    int WatcherGetChildren(const char* path,
            watcher_fn watcher,
            void* context,
            String_vector* strings);

    /// @brief Get current session cluster name.
    std::string GetName() const
    {
        return m_cluster_name;
    }
    /// @brief Get session id.
    std::string GetId()
    {
        MutexLocker locker(m_mutex);
        return m_id;
    }
    /// @brief Get session status.
    SessionStatus GetStatus()
    {
        MutexLocker locker(m_mutex);
        return m_status;
    }
    /// @brief Get watcher manager.
    WatcherManager* GetWatcherManager() {
        return &m_watcher_manager;
    }

private:
    void UpdateId();
    void OnSessionEvent(int event);
    bool IsRecoverableError(int error_code);
    void CloseHandle(zhandle_t* handle);
    FRIEND_TEST(ZookeeperClient, DnsFailureTest);
    static bool UpdateCacheFile();
    static bool ResolveIpFromCache(const std::string& cluster, std::string* cluster_ips);
    static bool ResolveServerAddress(const std::string& cluster, std::string* ips);

private:
    Mutex         m_mutex;              // mutex to protect handle change.
    std::string   m_cluster_name;       // zk cluster name, like: tjd1.zk.oa.com:2181
    SessionStatus m_status;             // session status
    zhandle_t*    m_handle;             // zookeeper handle of the session
    clientid_t*   m_client_id;          // current client id
    std::string   m_id;                 // session id, caculated from client id.
    AutoResetEvent  m_connect_event;    // connect event.

    bool          m_readonly;           // if current session connects to a proxy server.
    int           m_timeout;            // current session timeout, in milliseconds.
    std::string   m_credential;         // client credential
    // user session events callback
    Closure<void, std::string, int>* m_session_event_handler;
    // session event callback
    Closure<void, int>* m_callback;
    WatcherManager m_watcher_manager;   // watcher manager
};

} // namespace common

#endif // COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_SESSION_H
