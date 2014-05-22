// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author: Yubing Yin(yubingyin@tencent.com)

#ifndef COMMON_ZOOKEEPER_SDK_ZK_OPERATION_H
#define COMMON_ZOOKEEPER_SDK_ZK_OPERATION_H

#include <string>
#include <vector>

#include "common/base/uncopyable.h"
#include "common/system/concurrency/atomic/atomic.hpp"
#include "common/zookeeper/sdk/zk_watch.h"
#include "common/zookeeper/sdk/zookeeper.h"

namespace common
{

namespace zookeeper
{

enum OperationType
{
    kOperationNotify            = 0,
    kOperationCreate            = 1,
    kOperationDelete            = 2,
    kOperationExists            = 3,
    kOperationGetData           = 4,
    kOperationSetData           = 5,
    kOperationGetAcl            = 6,
    kOperationSetAcl            = 7,
    kOperationGetChildren       = 8,
    kOperationSync              = 9,
    kOperationPing              = 11,
    kOperationGetChildrenStat   = 12,
    kOperationClose             = -11,
    kOperationSetAuth           = 100,
    kOperationSetWatches        = 101,
};

enum SpecialOperationId
{
    kOperationIdWatchedEvent    = -1,
    kOperationIdPing            = -2,
    kOperationIdAuth            = -4,
    kOperationIdSetWatches      = -8,
};

class Operation
{
    DECLARE_UNCOPYABLE(Operation);

public:
    Operation(int32_t id, OperationType type, const char* buffer, int length)
        : m_id(id), m_type(type), m_is_sync(false),
          m_buffer(buffer), m_length(length), m_watcher(NULL) {}

    Operation(int32_t id, OperationType type, const std::string& path, bool is_sync,
              const char* buffer, int length, Watcher* watcher = NULL)
        : m_id(id), m_type(type), m_path(path), m_is_sync(is_sync),
          m_buffer(buffer), m_length(length), m_watcher(watcher) {}

    ~Operation()
    {
        FreeBuffer();
    }

    void FreeBuffer()
    {
        if (m_buffer) {
            delete m_buffer;
            m_buffer = NULL;
            m_length = 0;
        }
    }

    const std::string& GetPath() const
    {
        return m_path;
    }

    const char* GetBuffer() const
    {
        return m_buffer;
    }

    int GetLength() const
    {
        return m_length;
    }

    int32_t GetId() const
    {
        return m_id;
    }

    bool IsSync() const
    {
        return m_is_sync;
    }

    Watcher* GetWatcher() const
    {
        return m_watcher;
    }

private:
    int32_t         m_id;
    OperationType   m_type;
    std::string     m_path;
    bool            m_is_sync;
    const char*     m_buffer;
    int             m_length;
    Watcher*        m_watcher;
};

class OperationCreator
{
    DECLARE_UNCOPYABLE(OperationCreator);

public:
    OperationCreator();

    void DeleteOperation(Operation* operation);

    Operation* CreateOpCreate(const std::string& path, const std::string& data, int flag,
                              const ZooKeeperAclVector& acls, bool is_sync);

    Operation* CreateOpDelete(const std::string& path, int version, bool is_sync);

    Operation* CreateOpExists(const std::string& path, bool is_sync, Watcher* watcher = NULL);

    Operation* CreateOpGetData(const std::string& path, bool is_sync, Watcher* watcher = NULL);

    Operation* CreateOpSetData(const std::string& path, const std::string& data, int version,
                               bool is_sync);

    Operation* CreateOpGetAcl(const std::string& path, bool is_sync);

    Operation* CreateOpSetAcl(const std::string& path, const ZooKeeperAclVector& acls, int version,
                              bool is_sync);

    Operation* CreateOpGetChildren(const std::string& path, bool is_sync, Watcher* watcher = NULL);

    Operation* CreateOpGetChildrenStat(const std::string& path, bool is_sync,
                                       Watcher* watcher = NULL);

    Operation* CreateOpNotify(int timeout, int64_t last_txn_id, Session* session = NULL);

    Operation* CreateOpSetAuth(const std::string& scheme, const std::string& auth);

    Operation* CreateOpSetWatches(int64_t last_txn_id,
                                  const std::vector<std::string>& data_watches,
                                  const std::vector<std::string>& exist_watches,
                                  const std::vector<std::string>& child_watches);

    Operation* CreateOpSync(const std::string& path);

    Operation* CreateOpPing();

    Operation* CreateOpClose();

private:
    Atomic<int>     m_next_id;
};

} // namespace zookeeper

} // namespace common

#endif // COMMON_ZOOKEEPER_SDK_ZK_OPERATION_H
