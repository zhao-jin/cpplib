// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef COMMON_ZOOKEEPER_CLIENT_WATCHER_MANAGER_H
#define COMMON_ZOOKEEPER_CLIENT_WATCHER_MANAGER_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "common/base/closure.h"
#include "common/system/concurrency/mutex.h"
#include "thirdparty/zookeeper/zookeeper.h"

namespace common {

class ZookeeperSession;

class WatcherManager
{
public:
    // Closure param: <path, event>
    typedef Closure<void, std::string, int> WatcherCallback;
    typedef std::map<std::string, std::set<WatcherCallback*> > WatcherMap;

    WatcherManager() {}
    ~WatcherManager() {}

    int AddGetWatcher(
            ZookeeperSession* session,
            const std::string& path,
            watcher_fn func,
            WatcherCallback* cb,
            char* buffer,
            int* length,
            Stat* stat);

    int AddExistWatcher(
            ZookeeperSession* session,
            const std::string& path,
            watcher_fn func,
            WatcherCallback* cb,
            Stat* stat);

    int AddChildrenWatcher(
            ZookeeperSession* session,
            const std::string& path,
            watcher_fn func,
            WatcherCallback* cb,
            String_vector* sub_nodes);

    void CollectWatchers(const std::string& path, int event, std::set<WatcherCallback*>* watchers);
    void ProcessResidentWatcher(int event);

private:
    Mutex m_mutex;
    WatcherMap m_get_watchers;
    WatcherMap m_exist_watchers;
    WatcherMap m_children_watchers;
};

} // namespace common

#endif // COMMON_ZOOKEEPER_CLIENT_WATCHER_MANAGER_H

