// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author: Yubing Yin(yubingyin@tencent.com)

#ifndef COMMON_ZOOKEEPER_SDK_ZK_EVENT_H
#define COMMON_ZOOKEEPER_SDK_ZK_EVENT_H

#include <string>
#include <vector>

#include "common/zookeeper/sdk/zookeeper_data.h"
#include "common/zookeeper/sdk/zookeeper_watch.h"

namespace common
{

namespace zookeeper
{

class Event
{
public:
    virtual ~Event() {}

    virtual void Execute() = 0;
};

class WatchEvent : public Event
{
public:
    virtual void Execute();

    WatchEvent(EventType type, SessionState state, const std::string& path)
        : m_type(type), m_session_state(state), m_path(path) {}

    Watchers* GetWatchers()
    {
        return &m_watchers;
    }

private:
    EventType       m_event_type;
    SessionState    m_session_state;
    std::string     m_path;
    Watchers        m_watchers;
};

class CreateEvent : public Event {
public:
    virtual void Execute();

    CreateEvent(int ret_code, const std::string& path, const std::string& created_path,
                CreateCallback* callback)
        : m_ret_code(ret_code), m_path(path), m_created_path(created_path), m_callback(callback) {}

private:
    int             m_ret_code;
    std::string     m_path;
    std::string     m_created_path;
    CreateCallback* m_callback;
};

class StatEvent : public Event {
public:
    virtual void Execute();

    StatEvent(int ret_code, const std::string& path, const NodeStat& stat, StatCallback* callback)
        : m_ret_code(ret_code), m_path(path), m_stat(stat), m_callback(callback) {}

private:
    int             m_ret_code;
    std::string     m_path;
    NodeStat        m_stat;
    StatCallback*   m_callback;
};

class GetDataEvent : public Event {
public:
    virtual void Execute();

    GetDataEvent(int ret_code, const std::string& path, const std::string& data,
                 const NodeStat& stat, DataCallback* callback)
        : m_ret_code(ret_code), m_path(path), m_data(data), m_stat(stat), m_callback(callback) {}

private:
    int             m_ret_code;
    std::string     m_path;
    std::string     m_data;
    NodeStat        m_stat;
    DataCallback*   m_callback;
};

class GetAclEvent : public Event {
public:
    virtual void Execute();

    GetAclEvent(int ret_code, const std::string& path, const AclVector& acls,
                const NodeStat& stat, AclCallback* callback)
        : m_ret_code(ret_code), m_path(path), m_acls(acls), m_stat(stat), m_callback(callback) {}

private:
    int             m_ret_code;
    std::string     m_path;
    AclVector       m_acls;
    NodeStat        m_stat;
    AclCallback*    m_callback;
};

class GetChildrenEvent : public Event {
public:
    virtual void Execute();

    GetChildrenEvent(int ret_code, const std::string& path,
                     const std::vector<std::string>& children, ChildrenCallback* callback)
        : m_ret_code(ret_code), m_path(path), m_children(children), m_callback(callback) {}

private:
    int                         m_ret_code;
    std::string                 m_path;
    std::vector<std::string>    m_children;
    ChildrenCallback*           m_callback;
};

class GetChildrenStatEvent : public Event {
public:
    virtual void Execute();

    GetChildrenStatEvent(int ret_code, const std::string& path,
                         const std::vector<std::string>& children, const NodeStat& stat,
                         ChildrenStatCallback* callback)
        : m_ret_code(ret_code), m_path(path), m_children(children), m_stat(stat),
          m_callback(callback) {}

private:
    int                         m_ret_code;
    std::string                 m_path;
    std::vector<std::string>    m_children;
    NodeStat                    m_stat;
    ChildrenCallback*           m_callback;
};

} // namespace zookeeper

} // namespace common

#endif // COMMON_ZOOKEEPER_SDK_ZK_EVENT_H
