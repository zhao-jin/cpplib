// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: HUANG Dongping <dphuang@tencent.com>
// Created: 05/24/11
// Description:

#ifndef POPPY_RPC_CLIENT_IMPL_H
#define POPPY_RPC_CLIENT_IMPL_H
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "common/base/stdext/shared_ptr.h"
#include "common/net/http/http_client.h"
#include "common/system/concurrency/mutex.h"
#include "common/system/concurrency/thread_pool.h"
#include "common/system/timer/timer_manager.h"

#include "poppy/rpc_channel_option_info.pb.h"
#include "poppy/rpc_client.h"
#include "poppy/rpc_client_option_info.pb.h"

namespace common {
class CredentialGenerator;
} // namespace common

namespace poppy {

// using namespace common;

class ResolverFactory;
class RpcChannel;
class RpcChannelSwig;
class RpcChannelImpl;
class RpcRequestQueue;

typedef stdext::shared_ptr<RpcChannelImpl> RpcChannelImplPtr;

// RpcClient serves RpcChannelImpl with the following featuers:
//
// 1. Provide a thread pool (m_thread_pool)
// 2. Provide a timer (m_timer_manager)
// 3. Provide HTTP connection ( : public HttpClient)
// 4. Provide a RpcChannelImpl factory (protected by m_channel_mutex).
//
// TODO(yiwang): We might need to decompose this complex function
//               mixture into components.
//
class RpcClientImpl : public HttpClient, public stdext::enable_shared_from_this<RpcClientImpl>
{
    DECLARE_UNCOPYABLE(RpcClientImpl);
    friend class RpcChannelImpl;
    friend class RpcHttpChannel;
    friend class RpcConnection;
    friend class RpcChannelSwig;
    friend class RpcRequestQueue;

public:
    // You can also specify an optional netframe pointer, if you want rpc client
    // delete netframe automatically, you should set options.own_netframe to true
    RpcClientImpl(const RpcClientOptions& options,
                  netframe::NetFrame* netframe);

    RpcClientImpl(netframe::NetFrame* netframe,
                  bool own_netframe,
                  uint32_t channel_cache_size);

    explicit RpcClientImpl(int num_threads);

    virtual ~RpcClientImpl();

    static stdext::shared_ptr<RpcClientImpl> DefaultInstance();

    // Shutdown is used to stop the rpc client. Then for all channels created with
    // this RpcClient will switch to Shutdown status and cancel all their pending
    // requests. This client can not be used any more.
    // If Shutdown is not called, the channels will call Shutdown in their dtors
    // automatically.
    void Shutdown(bool wait_all_pending_request_finish);

    void GetMemoryUsage(MemoryUsage* memory_usage) const;

public:
    static const int kDefaultChannelCacheSize = 10;

private:
    void ShutdownAllChannels(bool wait_all_pending_request_finish);

    ThreadPool* GetThreadPool()
    {
        return &m_thread_pool;
    }

    void OnChannelAddressChanged(std::string channel_name,
                                 std::vector<std::string> addresses);

    TimerManager* mutable_timer_manager();

    RpcChannelImplPtr GetChannelImpl(const std::string& name,
                                     common::CredentialGenerator* credential_generator = NULL,
                                     const RpcChannelOptions& options = RpcChannelOptions());

    void ReleaseChannelImpl();

    // Connect to a server.
    bool AsyncConnect(const std::string& address,
                      HttpHandler* handler,
                      const netframe::NetFrame::EndPointOptions& endpoint_options,
                      HttpConnection** http_connection);

    void IncreaseRequestMemorySize(size_t size);
    void DecreaseRequestMemorySize(size_t size);

private:
    // mutex to protect internal data structures.
    mutable Mutex m_channel_mutex;

    // Put timer manager at beginning, because the others, for example the
    // channels depend on it.
    TimerManager m_timer_manager;

    // Thread pool to run callbacks.
    ThreadPool m_thread_pool;

    // The following m_channel and m_channel_cache shoule be maitained with
    // m_channel_map together.
    // The impl pointer in m_channel_map iff they are in m_channel or
    // m_channel_cache. Those impl objects are in use is in m_channel, those
    // for cache is in m_channel_cache.
    // We use ref_counted objects in m_channel and m_channel_cache but with
    // raw pointer in m_channel_map. Then when impl objects in m_channel with
    // reference count 1, we know that there isn't more object reference to it
    // so we can move it into m_channel_cache.
    typedef std::set<RpcChannelImpl*> RpcChannelSet;

    // Channels that hold by users
    RpcChannelSet m_channels;

    // Channels that not hold by user
    RpcChannelSet m_channel_cache;
    uint32_t m_channel_cache_size;

    // mapping from hash value of addresses to channel impl.
    typedef std::map<uint64_t, RpcChannelImplPtr> RpcChannelMap;
    RpcChannelMap m_channel_map;

    // Address resolver.
    ResolverFactory* m_address_resolver;

    bool m_has_shutdown;

    mutable MemoryUsage m_memory_usage;
};

} // namespace poppy

#endif // POPPY_RPC_CLIENT_IMPL_H
