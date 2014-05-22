// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_NODE_H
#define COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_NODE_H

#include <string>
#include <vector>
#include "common/base/closure.h"
#include "common/zookeeper/client/zookeeper_acl.h"
#include "thirdparty/zookeeper/zookeeper.h"

namespace common {

class ZookeeperSession;

class ZookeeperNode {
public:
    static const int kMaxBufferLen = 1024 * 1024;

    /// Watcher callback, call back function should be defined as:
    /// void CallbackFunction(std::string path, int event);
    typedef Closure<void, std::string, int> WatcherCallback;

    /// Watcher callback, call back function should be defined as:
    /// void CallbackFunction(std::string path, int event, std::vector<std::string> old_children);
    typedef Closure<void, std::string, int, std::vector<std::string> > ChildrenWatcherCallback;

    virtual ~ZookeeperNode() {}

    /// @brief Create current node.
    /// @param data node data.
    /// @param flags 0 (normal node) / ZOO_EPHEMERAL / ZOO_SEQUENCE
    /// @retval error_code return ZK_OK if create successfully; ZK_NODEEXISTS when node already
    ///  exists, other error code please see definations in zookeeer_types.h.
    virtual int Create(const std::string& data = "", int flags = 0);

    /// @brief Recursively create the node. if ancestors not exist, create them with empty data.
    /// @param data current node data.
    /// @param flags 0 (normal node) / ZOO_EPHEMERAL / ZOO_SEQUENCE
    /// @retval error_code same as Create
    virtual int RecursiveCreate(const std::string& data = "", int flags = 0);

    /// @brief Get current node contents and stat.
    /// @param contents output node contents.
    /// @param stat output node stat.
    /// @param callback when callback is not NULL, it means you add a watcher.
    /// callback is a oneshot closure.
    /// when node status changes, callback will be called, the callback function should like:
    /// void Callback(std::string path, int state);
    /// @retval error_code
    virtual int GetContent(
        std::string* contents, Stat* stat = NULL, WatcherCallback* callback = NULL);

    /// This interface is used in php client.
    virtual int GetContentAsJson(std::vector<std::string>* contents);

    /// @brief Set current node contents.
    /// @param value node value
    /// @param version the expected version of the node
    /// @retval error_code
    virtual int SetContent(const std::string& value, int version = -1);

    /// @brief Test if current node exists.
    /// @param callback when callback is not NULL, it means you add a watcher.
    /// callback is a oneshot closure.
    /// when node status changes, callback will be called, the callback function should like:
    /// void Callback(std::string path, int state);
    /// @retval error_code
    virtual int Exists(WatcherCallback* callback = NULL);

    /// @brief Delete current zookeeper node.
    /// @param version the expected version of the node
    /// @retval error_code
    virtual int Delete(int version = -1);

    /// @brief Recursively delete a node.
    /// @retval error_code
    virtual int RecursiveDelete();

    /// @brief Get a specified child contents of current node.
    /// @param child_name child name
    /// @param contents output child contents
    /// @param stat output child node stat.
    /// @retval error_code
    virtual int GetChildContent(
            const std::string& child_name,
            std::string* contents,
            Stat* stat = NULL);

    /// @brief set value to a specified child.
    /// @param child_name child name
    /// @param contents node contents
    /// @param version the expected version of the node
    /// @retval error_code
    virtual int SetChildContent(
            const std::string& child_name,
            const std::string& contents,
            int  version = -1);

    /// @brief List all children name of current node.
    /// @param children output vector which stores the children names.
    /// @param callback when callback is not NULL, it means you add a watcher.
    /// when children add or leave or current watched node is deleted, callback will be called.
    /// The callback function should like:
    /// void Callback(string path, int error_code, std::vector<std::string> old_children);
    /// @retval error_code
    virtual int GetChildren(
        std::vector<std::string>* children, ChildrenWatcherCallback* callback = NULL);

    /// @brief Judge if this node is locked.
    /// @param error_code
    /// @retval true if locked, false when unlocked or call failed (error code indicates failure
    /// reason in such case).
    virtual bool IsLocked(int* error_code = NULL);

    /// @brief Lock curent node. it's a blocking operation. It will wait until lock acquired.
    /// @retval error_code
    virtual int Lock();

    /// @brief Try to lock current node, return immediately.
    /// @retval error_code ZK_OK if success.
    virtual int TryLock();

    /// @brief Try to lock current node, it's asynchronous.
    /// @param callback not matter success or failure, the callback will be excuted.
    /// callback is a oneshot closure.
    /// The callback should like:
    /// void Callback(std::string path, int error_code);
    /// When locked successfully, error_code is ZK_OK.
    virtual void AsyncLock(Closure<void, std::string, int>* callback);

    /// @brief Get node acl.
    /// @param acls output node acls
    /// @param stat output node stat
    /// @retval error_code
    int GetAcl(ZookeeperAcl* acls, Stat* stat = NULL);

    /// @brief Unlock current node.
    /// @retval error_code
    virtual int Unlock();

    virtual std::string GetPath() const {
        return m_path;
    }

protected:
    ZookeeperNode() : m_session(NULL) {}

    /// @brief sort lock nodes by sequential number.
    void SortLockNodes(std::vector<std::string>* nodes);
    /// @brief find current session lock node under current path.
    bool FindOrCreateLockNode(std::string* node_name, int* error_code = NULL);
    /// @brief find precursor of current lock node.
    int FindPrecursor(std::string* precursor, int* error_code);

    friend class ZookeeperClient;
    friend class ZookeeperSession;
    void SetPath(const std::string& path)
    {
        m_path = path;
    }
    void SetSession(ZookeeperSession* session)
    {
        m_session = session;
    }
    /// node path, like: /zk/tjd1/xcube
    std::string m_path;
    /// node session
    ZookeeperSession* m_session;
};

} // namespace common

#endif // COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_NODE_H
