// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: rabbitliu <rabbitliu@tencent.com>
// Created: 09/21/11

#ifndef COMMON_ZOOKEEPER_PERF_TEST_WATCHER_H
#define COMMON_ZOOKEEPER_PERF_TEST_WATCHER_H

#include "common/system/concurrency/atomic/atomic.hpp"
#include "common/zookeeper/perf_test/zk_client.h"
#include "thirdparty/glog/logging.h"
#include "zookeeper/zookeeper.h"

// namespace common {

Atomic<int> g_delete_nodes = 0;
Atomic<int> g_change_nodes = 0;

void GlobalWatcher(zhandle_t* zh, int type, int state, const char *path, void *context)
{
    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_EXPIRED_SESSION_STATE) {
            ZkClient* zk_client = static_cast<ZkClient*>(context);
            zk_client->GetSyncEvent().Set();
            LOG(FATAL) << "zoo session expired";
        } else if (state == ZOO_CONNECTED_STATE) {
            ZkClient* zk_client = static_cast<ZkClient*>(context);
            zk_client->GetSyncEvent().Set();
        }
    } else if (type == ZOO_CHANGED_EVENT) {
        VLOG(3) << "node data changed";
        ++g_change_nodes;
        if (g_change_nodes % 10000 == 0) {
            LOG(INFO) << "g_change_nodes: " << g_change_nodes;
        }
    }
    else if (type == ZOO_DELETED_EVENT) {
        VLOG(3) << "node deleted";
        ++g_delete_nodes;
        if (g_delete_nodes % 10000 == 0) {
            LOG(INFO) << "g_delete_nodes: " << g_delete_nodes;
        }
    }
    else if (type == ZOO_CHILD_EVENT) {
        VLOG(3) << "node children changed";
    } else if (type == ZOO_CREATED_EVENT) {
        VLOG(3) << "node created";
    }
}

// } // namespace common

#endif // COMMON_ZOOKEEPER_PERF_TEST_WATCHER_H
