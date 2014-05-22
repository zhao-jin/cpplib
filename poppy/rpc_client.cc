// Copyright 2011, Tencent Inc.
// Author: HUANG Dongping <dphuang@tencent.com>
// Created: 05/24/11
// Description:

#include "poppy/rpc_client.h"
#include "poppy/rpc_client_impl.h"
#include "poppy/set_typhoon_tos_flag.h"
namespace poppy {

RpcClient::RpcClient() : m_impl(RpcClientImpl::DefaultInstance())
{
    SetTyphoonTosIfNecessary();
}

RpcClient::RpcClient(const RpcClientOptions& options,
                     netframe::NetFrame* netframe)
    : m_impl(new RpcClientImpl(options, netframe))
{
    SetTyphoonTosIfNecessary();
}

RpcClient::RpcClient(
    netframe::NetFrame* netframe,
    bool own_netframe,
    uint32_t cache_limit)
    : m_impl(new RpcClientImpl(netframe, own_netframe, cache_limit))
{
    SetTyphoonTosIfNecessary();
}

RpcClient::RpcClient(int num_threads)
    : m_impl(new RpcClientImpl(num_threads))
{
    SetTyphoonTosIfNecessary();
}

RpcClient::~RpcClient()
{
}

void RpcClient::Shutdown(bool wait_all_pending_request_finish)
{
    m_impl->Shutdown(wait_all_pending_request_finish);
}

void RpcClient::GetMemoryUsage(MemoryUsage* memory_usage) const
{
    m_impl->GetMemoryUsage(memory_usage);
}

} // namespace poppy

