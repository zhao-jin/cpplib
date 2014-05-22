// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_CLIENT_H
#define COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_CLIENT_H

#include <map>
#include <string>
#include "common/base/closure.h"
#include "common/base/deprecate.h"
#include "common/base/uncopyable.h"
#include "common/system/concurrency/mutex.h"
#include "common/zookeeper/client/zookeeper_node.h"
#include "common/zookeeper/client/zookeeper_types.h"
#include "thirdparty/glog/logging.h"

namespace common {

class ZookeeperSession;

class ZookeeperClient
{
    DECLARE_UNCOPYABLE(ZookeeperClient);
public:
    struct Options
    {
    public:
        Options(const std::string& c = "", bool rdo = false, int to = 4000)
            : credential(c), readonly(rdo), timeout(to) {}
        ~Options() {}
        /// Client credential
        std::string credential;
        /// If Client is readonly
        bool readonly;
        /// Operation timeout, in millisecond.
        int  timeout;
    };

    /// @param options Zookeeper client options.
    /// @param handler Session event handler, Permenant closure.
    /// void HandleSessionEvent(std::string cluster_name, int event_type).
    /// It will be deleted when ZookeeperClient destroys.
    explicit ZookeeperClient(const Options& options = Options(),
                             Closure<void, std::string, int>* handler = NULL);
    ~ZookeeperClient();

    void ChangeIdentity(const std::string& credential);

    /// @brief Open a zookeeper node.
    /// @param path node path, e.g. /zk/tjd1/xcube
    /// The cluster name can be deduced from the second section of the path, e.g., tjd1
    /// @param error_code
    /// @retval node  ZookeeperNode pointer pointed to the node opened. NULL if failed.
    /// It must be deleted when it's not used any more.
    ZookeeperNode* Open(const std::string& path, int* error_code = NULL);

    /// @brief Create a zookeeper node, if node exists, open it.
    /// @param path node path, e.g. /zk/tjd1/xcube
    /// The cluster name can be deduced from the second section of the path, e.g., tjd1
    /// @param data node data.
    /// @param flags 0 (normal node) / ZOO_EPHEMERAL / ZOO_SEQUENCE
    /// @param error_code output error code if create op failed.
    /// @retval node  ZookeeperNode pointer pointed to the node opened. NULL if failed.
    /// It must be deleted when it's not used any more.
    ZookeeperNode* Create(const std::string& path, const std::string& data = "",
                          int flags = 0, int* error_code = NULL);

    /// @brief This function is used for acl, it encrypt plain text id password.
    static bool Encrypt(const std::string& id_passwd, std::string* encrypted_passwd);

    /// @brief Close all connected zookeeper sessions.
    void Close();

    /// @brief Open a zookeeper node. This function is for special use.
    /// e.g, open a root node or open a node on a standalone zookeeper server.
    /// @param path node full path with cluster name, e.g. tjd1.zk.oa.com:2181/zk/tjd1/xcube
    /// @param error_code
    /// @retval node  ZookeeperNode pointer pointed to the node opened. NULL if failed.
    /// It must be deleted when it's not used any more.
    ZookeeperNode* OpenWithFullPath(const std::string& path, int* error_code = NULL);

private:
    /// @brief Find a session.
    ZookeeperSession* FindOrMakeSession(const std::string& cluster_name);
    /// @brief Get proxy name for real zk cluster.
    bool GetProxyName(const std::string& cluster_name, std::string* proxy) const;
    /// @brief Get cluster name from path.
    bool GetClusterName(const std::string& path, std::string* cluster_name) const;
    /// @brief Get ret code for mock zookeeper node from path.
    bool GetMockRetCode(const std::string& path, int* ret_code);

private:
    Mutex m_mutex;
    Options m_options;
    std::map<std::string, ZookeeperSession*> m_sessions;
    /// Session event handler, Permenant closure.
    /// void HandleSessionEvent(std::string cluster_name, int event_type).
    Closure<void, std::string, int>* m_session_event_handler;
};

} // namespace common

#endif // COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_CLIENT_H
