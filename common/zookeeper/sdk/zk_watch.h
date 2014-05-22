// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author: Yubing Yin(yubingyin@tencent.com)

#ifndef COMMON_ZOOKEEPER_SDK_ZK_WATCH_H
#define COMMON_ZOOKEEPER_SDK_ZK_WATCH_H

#include <map>
#include <string>
#include <vector>

#include "common/base/uncopyable.h"
#include "common/zookeeper/sdk/zookeeper.h"

namespace common
{

namespace zookeeper
{

class WatchManager
{
    DECLARE_UNCOPYABLE(WatchManager);

public:
    typedef std::vector<Watcher*> Watchers;

public:
    void RegisterDataWatch(const std::string& path, Watcher* watcher)
    {
        RegisterWatch(path, watcher, &m_data_watches);
    }

    void RegisterExistWatch(const std::string& path, Watcher* watcher)
    {
        RegisterWatch(path, watcher, &m_exist_watches);
    }

    void RegisterChildWatch(const std::string& path, Watcher* watcher)
    {
        RegisterWatch(path, watcher, &m_child_watches);
    }

    void CollectWatches(EventType event_type, const std::string& path, Watchers* watchers);

    void GetAllWatches(std::vector<std::string>* data_watches,
                       std::vector<std::string>* exist_watches,
                       std::vector<std::string>* child_watches);

private:
    typedef std::map<std::string, Watchers>  WatchMap;

    static void RegisterWatch(const std::string& path, Watcher* watcher, WatchMap* watches)
    {
        (*watches)[path].push_back(watcher);
    }

private:
    WatchMap    m_data_watches;
    WatchMap    m_exist_watches;
    WatchMap    m_child_watches;
};

} // namespace zookeeper

} // namespace common

#endif // COMMON_ZOOKEEPER_SDK_ZK_WATCH_H
