// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author: Yubing Yin(yubingyin@tencent.com)

#ifndef COMMON_ZOOKEEPER_SDK_ZK_CONNECTION_H
#define COMMON_ZOOKEEPER_SDK_ZK_CONNECTION_H

#include <string>
#include <vector>

#include "common/base/uncopyable.h"
#include "common/system/net/socket_address.h"

namespace common
{

namespace zookeeper
{

class ZooKeeperConnection
{
    DECLARE_UNCOPYABLE(ZooKeeperConnection);

public:
    ZooKeeperConnection()
        : m_session_timeout(0), m_last_receive(0), m_last_send(0),
          m_last_ping(0), m_is_connected(false)
    {
        m_cur_server = m_server_list.end();
    }

    bool Init(const std::string& server_list, int timeout)
    {
        if (!ParseServerList(server_list)) {
            return false;
        }
        ShuffleServerList();
        m_cur_server = m_server_list.begin();
        m_session_timeout = timeout;
    }

    void UpdateReceiveTime(int64_t time)
    {
        m_last_receive = time;
    }

    void UpdateSendTime(int64_t time)
    {
        m_last_send = time;
    }

    void UpdatePingTime(int64_t time)
    {
        m_last_ping = time;
    }

    int64_t LastReceiveTime() const
    {
        return m_last_receive;
    }

    int64_t LastSendTime() const
    {
        return m_last_send;
    }

    int64_t LastPingTime() const
    {
        return m_last_ping;
    }

    void Disconnected()
    {
        m_is_connected = false;
    }

    void ConnectedOn(int64_t time)
    {
        m_is_connected = true;
        UpdateReceive(time);
        UpdateSend(time);
        UpdatePing(time);
    }

    bool HasNext() const
    {
        return m_cur_server != m_server_list.end();
    }

    void Reset()
    {
        m_cur_server = m_server_list.begin();
    }

    int GetLeaseTimeout(int64_t time)
    {
        return m_session_timeout * 2 / 3 - static_cast<int>(time - m_last_receive);
    }

    int GetConnectTimeout()
    {
        return m_session_timeout / static_cast<int>(m_server_list.size());
    }

    SocketAddressInet GetNext()
    {
        return *(m_cur_server++);
    }

private:
    bool ParseServerList(const std::string& server_list);

    void ShuffleServerList();

    typedef std::vector<SocketAddressInet> AddressList;

    typedef AddressList::const_iterator AddressListIter;

private:
    AddressList     m_server_list;
    AddressListIter m_cur_server;
    int             m_session_timeout;
    int64_t         m_last_receive;
    int64_t         m_last_send;
    int64_t         m_last_ping;
    bool            m_is_connected;
};

} // namespace zookeeper

} // namespace common

#endif // COMMON_ZOOKEEPER_SDK_ZK_CONNECTION_H
