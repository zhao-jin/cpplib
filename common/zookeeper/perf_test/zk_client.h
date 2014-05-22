// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: rabbitliu <rabbitliu@tencent.com>
// Created: 09/19/11
// Description:

#ifndef COMMON_ZOOKEEPER_PERF_TEST_ZK_CLIENT_H
#define COMMON_ZOOKEEPER_PERF_TEST_ZK_CLIENT_H

#include <string>
#include <vector>
#include "common/system/concurrency/event.h"
#include "common/system/concurrency/mutex.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/zookeeper/zookeeper.h"

// namespace common {

void GlobalWatcher(zhandle_t* zh, int type, int state, const char *path, void *context);

class ZkClient {
public:
    static const int kMaxBufferLen = 1024 * 1024;

    explicit ZkClient(const std::string& zk_host, bool auto_connect = true) {
        m_set_buffer = new char[kMaxBufferLen + 1];
        m_get_buffer = new char[kMaxBufferLen + 1];
        memset(m_set_buffer, 'A', kMaxBufferLen);
        m_host = zk_host;
        m_handle = NULL;
        if (auto_connect) {
            if (!Connect()) {
                LOG(FATAL) << "failed to connect to zookeeper server.";
            } else {
                VLOG(3) << "successfully connected to zk server.";
            }
        }
    }

    virtual ~ZkClient() {
        Close();
        if (m_get_buffer != NULL) {
            delete[] m_get_buffer;
            m_get_buffer = NULL;
        }
        if (m_set_buffer != NULL) {
            delete[] m_set_buffer;
            m_set_buffer = NULL;
        }
    }

    void Close() {
        if (m_handle) {
            zookeeper_close(m_handle);
            m_handle = NULL;
        }
    }

    bool Connect() {
        bool ret = true;
        int retry_num = 0;
        while (retry_num <= kMaxRetryTimes) {
            if (retry_num > 0) {
                LOG(WARNING) << "Connect timeout, retry: " << retry_num;
            }
            zoo_deterministic_conn_order(0);
            m_handle = zookeeper_init(m_host.c_str(), GlobalWatcher, 4000, 0, this, 0);
            if (m_handle == NULL) {
                LOG(FATAL) << "failed to call zookeeper init.";
            }
            ret = m_sync_event.Wait(4000);
            if (ret) {
                return true;
            }
            Close();
            retry_num++;
        }
        return ret;
    }

    // flags: 0, ZOO_EPHEMERAL, ZOO_SEQUENCE
    int CreateNode(const std::string& node_path, int node_size, int flags) {
        int ret = ZOK;
        int retry_num = 0;
        if (node_size == 0) {
            while (retry_num <= kMaxRetryTimes) {
                if (retry_num > 0) {
                    LOG(WARNING) << "Creat failed with error code: " << ret
                                 << ", retry: " << retry_num;
                }
                ret = zoo_create(m_handle,
                        node_path.c_str(),
                        NULL,
                        -1,
                        &ZOO_OPEN_ACL_UNSAFE,
                        flags,
                        m_get_buffer,
                        kMaxBufferLen);
                if (ret != ZOPERATIONTIMEOUT && ret != ZCONNECTIONLOSS && ret != ZSESSIONMOVED) {
                    return ret;
                }
                retry_num++;
            }
        } else {
            while (retry_num <= kMaxRetryTimes) {
                if (retry_num > 0) {
                    LOG(WARNING) << "Creat failed with error code: " << ret
                                 << ", retry: " << retry_num;
                }
                ret = zoo_create(m_handle,
                        node_path.c_str(),
                        m_set_buffer,
                        node_size,
                        &ZOO_OPEN_ACL_UNSAFE,
                        flags,
                        m_get_buffer,
                        kMaxBufferLen);
                if (ret != ZOPERATIONTIMEOUT && ret != ZCONNECTIONLOSS && ret != ZSESSIONMOVED) {
                    return ret;
                }
                retry_num++;
            }
        }
        return ret;
    }

    int Set(const std::string& path, int node_size) {
        if (node_size < 0 || node_size > 1024 * 1024) {
            VLOG(3) << "invalid node data size.";
            return -1;
        }
        int ret = ZOK;
        int retry_num = 0;
        while (retry_num <= kMaxRetryTimes) {
            if (retry_num > 0) {
                LOG(WARNING) << "Set failed with error code: " << ret << ", retry: " << retry_num;
            }
            ret = zoo_set(m_handle, path.c_str(), m_set_buffer, node_size, -1);
            if (ret != ZOPERATIONTIMEOUT && ret != ZCONNECTIONLOSS && ret != ZSESSIONMOVED) {
                return ret;
            }
            retry_num++;
        }
        return ret;
    }

    int Get(const std::string& path) {
        int len = kMaxBufferLen;
        struct Stat stat;
        int ret = ZOK;
        int retry_num = 0;
        while (retry_num <= kMaxRetryTimes) {
            if (retry_num > 0) {
                LOG(WARNING) << "Get failed with error code: " << ret << ", retry: " << retry_num;
            }
            ret = zoo_get(m_handle, path.c_str(), 0, m_get_buffer, &len, &stat);
            if (ret != ZOPERATIONTIMEOUT && ret != ZCONNECTIONLOSS && ret != ZSESSIONMOVED) {
                return ret;
            }
            retry_num++;
        }
        return ret;
    }

    int Delete(const std::string& path) {
        int ret = ZOK;
        int retry_num = 0;
        while (retry_num <= kMaxRetryTimes) {
            if (retry_num > 0) {
                LOG(WARNING) << "Delete failed with error code: " << ret
                             << ", retry: " << retry_num;
            }
            ret = zoo_delete(m_handle, path.c_str(), -1);
            if (ret != ZOPERATIONTIMEOUT && ret != ZCONNECTIONLOSS && ret != ZSESSIONMOVED) {
                return ret;
            }
            retry_num++;
        }
        return ret;
    }

    int SetWatch(const std::string& path, int watch_event_type) {
        struct Stat stat;
        int ret = -1;
        if (watch_event_type == ZOO_CREATED_EVENT || watch_event_type == ZOO_DELETED_EVENT) {
            ret = zoo_exists(m_handle, path.c_str(), 1, &stat);
        } else if (watch_event_type == ZOO_CHANGED_EVENT) {
            int len = kMaxBufferLen;
            ret = zoo_get(m_handle, path.c_str(), 1, m_get_buffer, &len, &stat);
        } else if (watch_event_type == ZOO_CHILD_EVENT) {
            struct String_vector vec = { 0, NULL };
            ret = zoo_get_children(m_handle, path.c_str(), 1, &vec);
        }
        return ret;
    }

    int GetChildren(const std::string& path, std::vector<std::string>* children) {
        children->clear();
        int ret = ZOK;
        int retry_num = 0;
        struct String_vector sub_nodes = {0, NULL};
        while (retry_num <= kMaxRetryTimes) {
            if (retry_num > 0) {
                LOG(WARNING) << "GetChildren failed with error code: " << ret
                             << ", retry: " << retry_num;
            }
            ret = zoo_get_children(m_handle, path.c_str(), 0, &sub_nodes);
            if (ret == ZOPERATIONTIMEOUT || ret == ZCONNECTIONLOSS || ret == ZSESSIONMOVED) {
                deallocate_String_vector(&sub_nodes);
                sub_nodes.count = 0;
                sub_nodes.data = NULL;
            } else {
                break;
            }
            retry_num++;
        }
        if (ret != ZOK) {
            deallocate_String_vector(&sub_nodes);
            return ret;
        }
        for (int i = 0; i < sub_nodes.count; ++i) {
            std::string child = sub_nodes.data[i];
            child = path + "/" + child;
            children->push_back(child);
        }
        deallocate_String_vector(&sub_nodes);
        return ret;
    }

    bool IsLeaf(const std::string& path) {
        int ret = -1;
        struct String_vector sub_nodes = {0, NULL};
        ret = zoo_get_children(m_handle, path.c_str(), 0, &sub_nodes);
        if (ret != ZOK) {
            deallocate_String_vector(&sub_nodes);
            return false;
        }
        return sub_nodes.count == 0;
    }

    AutoResetEvent& GetSyncEvent() {
        return m_sync_event;
    }

private:
    static const int kMaxRetryTimes = 3;
    AutoResetEvent m_sync_event;
    char* m_set_buffer;
    char* m_get_buffer;
    zhandle_t* m_handle;
    std::string m_host;
};

// } // namespace common

#endif // COMMON_ZOOKEEPER_PERF_TEST_ZK_CLIENT_H
