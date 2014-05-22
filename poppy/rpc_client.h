// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: HUANG Dongping <dphuang@tencent.com>
// Created: 05/24/11
// Description:

#ifndef POPPY_RPC_CLIENT_H
#define POPPY_RPC_CLIENT_H
#pragma once

#include "common/base/deprecate.h"
#include "common/base/stdext/shared_ptr.h"
#include "common/base/stdint.h"
#include "common/base/uncopyable.h"
#include "poppy/rpc_client_option_info.pb.h"

namespace common {
class CredentialGenerator;
} // namespace common

namespace netframe {
class NetFrame;
} // namespace netframe

// For unit test
namespace rpc_examples {
class RpcChannel;
}

namespace poppy {

class RpcClientImpl;

struct MemoryUsage
{
    size_t queued_size;
    size_t send_buffer_size;
    size_t receive_buffer_size;
};

class RpcClient
{
    DECLARE_UNCOPYABLE(RpcClient);
    friend class RpcChannel;
    friend class RpcChannelSwig;
    friend class rpc_examples::RpcChannel;

public:
    static const int kDefaultChannelCacheSize = 10;

public:
    // Obtain a default constructed RpcClient object.
    // NOTE: Default constructed RpcClient share the same underlay instance.
    // You can modify the options by command line flags.
    RpcClient();

    // You can also specify an optional netframe pointer, if you want rpc client
    // delete netframe automatically, you should set options.own_netframe to true
    explicit RpcClient(const RpcClientOptions& options,
                       netframe::NetFrame* netframe = NULL);

    DEPRECATED_BY(RpcClient(const RpcClientOptions& options, netframe::NetFrame* netframe))
    explicit RpcClient(netframe::NetFrame* netframe,
                       bool own_netframe = false,
                       uint32_t channel_cache_size = kDefaultChannelCacheSize);

    DEPRECATED_BY(RpcClient(const RpcClientOptions& options))
    explicit RpcClient(int num_threads);

    virtual ~RpcClient();

    void Shutdown(bool wait_all_pending_request_finish = false);

    void GetMemoryUsage(MemoryUsage* memory_usage) const;

private:
    const stdext::shared_ptr<RpcClientImpl>& impl() const
    {
        return m_impl;
    }

private:
    stdext::shared_ptr<RpcClientImpl> m_impl;
};

} // namespace poppy

#endif // POPPY_RPC_CLIENT_H
