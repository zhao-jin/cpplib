// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "common/zookeeper/client/watcher_manager.h"
#include "common/zookeeper/client/zookeeper_session.h"
#include "common/zookeeper/client/zookeeper_types.h"
#include "thirdparty/glog/logging.h"

namespace common {

int WatcherManager::AddGetWatcher(
        ZookeeperSession* session,
        const std::string& path,
        watcher_fn func,
        WatcherCallback* callback,
        char* buffer,
        int* length,
        Stat* stat)
{
    MutexLocker locker(m_mutex);
    int error_code = session->WatcherGet(path.c_str(), func, session, buffer, length, stat);
    if (error_code != ZK_OK) {
        return error_code;
    }
    std::map<std::string, std::set<WatcherCallback*> >::iterator iter;
    iter = m_get_watchers.find(path);
    if (iter == m_get_watchers.end()) {
        std::set<WatcherCallback*> s;
        s.insert(callback);
        m_get_watchers[path] = s;
    } else {
        iter->second.insert(callback);
    }
    return error_code;
}

int WatcherManager::AddExistWatcher(
        ZookeeperSession* session,
        const std::string& path,
        watcher_fn func,
        WatcherCallback* callback,
        Stat* stat)
{
    MutexLocker locker(m_mutex);
    int error_code = session->WatcherExists(path.c_str(), func, session, stat);
    if (error_code != ZK_OK && error_code != ZK_NONODE) {
        return error_code;
    }
    std::map<std::string, std::set<WatcherCallback*> >::iterator iter;
    iter = m_exist_watchers.find(path);
    if (iter == m_exist_watchers.end()) {
        std::set<WatcherCallback*> s;
        s.insert(callback);
        m_exist_watchers[path] = s;
    } else {
        iter->second.insert(callback);
    }
    return error_code;
}

int WatcherManager::AddChildrenWatcher(
        ZookeeperSession* session,
        const std::string& path,
        watcher_fn func,
        WatcherCallback* callback,
        String_vector* sub_nodes)
{
    MutexLocker locker(m_mutex);
    int error_code = session->WatcherGetChildren(path.c_str(), func, session, sub_nodes);
    if (error_code != ZK_OK) {
        return error_code;
    }
    std::map<std::string, std::set<WatcherCallback*> >::iterator iter;
    iter = m_children_watchers.find(path);
    if (iter == m_children_watchers.end()) {
        std::set<WatcherCallback*> s;
        s.insert(callback);
        m_children_watchers[path] = s;
    } else {
        iter->second.insert(callback);
    }
    return error_code;
}

void WatcherManager::CollectWatchers(const std::string& path,
                                     int event,
                                     std::set<WatcherCallback*>* watchers)
{
    watchers->clear();
    MutexLocker locker(m_mutex);
    std::map<std::string, std::set<WatcherCallback*> >::iterator iter;
    if (event == ZK_NODE_CREATED_EVENT) {
        iter = m_exist_watchers.find(path);
        if (iter != m_exist_watchers.end()) {
            watchers->swap(iter->second);
            m_exist_watchers.erase(iter);
        }
    } else if (event == ZK_NODE_DELETED_EVENT || event == ZK_NODE_CHANGED_EVENT) {
        iter = m_get_watchers.find(path);
        if (iter != m_get_watchers.end()) {
            watchers->swap(iter->second);
            m_get_watchers.erase(iter);
        }

        iter = m_exist_watchers.find(path);
        if (iter != m_exist_watchers.end()) {
            watchers->insert(iter->second.begin(), iter->second.end());
            m_exist_watchers.erase(iter);
        }
        // GetChildren watcher should be triggered when node deleted.
        if (event == ZK_NODE_DELETED_EVENT) {
            iter = m_children_watchers.find(path);
            if (iter != m_children_watchers.end()) {
                watchers->insert(iter->second.begin(), iter->second.end());
                m_children_watchers.erase(iter);
            }
        }
    } else if (event == ZK_CHILD_CHANGED_EVENT) {
        iter = m_children_watchers.find(path);
        if (iter != m_children_watchers.end()) {
            watchers->swap(iter->second);
            m_children_watchers.erase(iter);
        }
    }
}

void WatcherManager::ProcessResidentWatcher(int event)
{
    std::map<std::string, std::set<WatcherCallback*> > get_watchers;
    std::map<std::string, std::set<WatcherCallback*> > exist_watchers;
    std::map<std::string, std::set<WatcherCallback*> > children_watchers;
    {
        MutexLocker locker(m_mutex);
        get_watchers.swap(m_get_watchers);
        exist_watchers.swap(m_exist_watchers);
        children_watchers.swap(m_children_watchers);
    }
    std::map<std::string, std::set<WatcherCallback*> >::iterator iter;
    for (iter = get_watchers.begin(); iter != get_watchers.end(); ++iter) {
        std::set<WatcherCallback*>& s = iter->second;
        std::set<WatcherCallback*>::iterator callback;
        for (callback = s.begin(); callback != s.end(); ++callback) {
            (*callback)->Run(iter->first, event);
        }
    }
    for (iter = exist_watchers.begin(); iter != exist_watchers.end(); ++iter) {
        std::set<WatcherCallback*>& s = iter->second;
        std::set<WatcherCallback*>::iterator callback;
        for (callback = s.begin(); callback != s.end(); ++callback) {
            (*callback)->Run(iter->first, event);
        }
    }
    for (iter = children_watchers.begin(); iter != children_watchers.end(); ++iter) {
        std::set<WatcherCallback*>& s = iter->second;
        std::set<WatcherCallback*>::iterator callback;
        for (callback = s.begin(); callback != s.end(); ++callback) {
            (*callback)->Run(iter->first, event);
        }
    }
}

} // namespace common
