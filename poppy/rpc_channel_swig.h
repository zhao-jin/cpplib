// Copyright 2011, Tencent Inc.
// Author: Eric Liu <ericliu@tencent.com>

#ifndef POPPY_RPC_CHANNEL_SWIG_H
#define POPPY_RPC_CHANNEL_SWIG_H

#include <map>
#include <string>

#include "poppy/rpc_channel_impl.h"
#include "poppy/rpc_client.h"

namespace poppy {

// using namespace common;

class ResponseHandler
{
public:
    virtual ~ResponseHandler() {}
    virtual void Run(const std::string& response_data) = 0;
    void InnerRun(const std::string* response_data_ptr);

protected:
    ResponseHandler() {}
};

// The client rpc channel for swig wrapper.
class RpcChannelSwig
{
public:
    RpcChannelSwig(RpcClient* rpc_client, const std::string& name);
    virtual ~RpcChannelSwig();

    std::string RawCallMethod(RpcController* rpc_controller,
                              const std::string& request_data,
                              ResponseHandler* done);

protected:
    static void OnResponseReceived(RpcClient* rpc_client,
                                   RpcController* rpc_controller,
                                   std::string* response_data_ptr,
                                   ResponseHandler* handler,
                                   const StringPiece* message_data);

    stdext::shared_ptr<RpcChannelImpl> m_channel_impl;
    RpcClient* m_rpc_client;
};

} // namespace poppy

#endif // POPPY_RPC_CHANNEL_SWIG_H
