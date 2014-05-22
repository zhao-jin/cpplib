// Copyright 2011, Tencent Inc.
// Author: Yi Wang <yiwang@tencent.com>
//
#ifndef POPPY_RPC_CHANNEL_H
#define POPPY_RPC_CHANNEL_H

#include <string>
#include <vector>

#include "common/base/scoped_ptr.h"
#include "poppy/rpc_channel_interface.h"
#include "poppy/rpc_channel_option_info.pb.h"
#include "poppy/rpc_controller.h"

namespace common {
class CredentialGenerator;
} // namespace common

namespace poppy {

// Max single RPC request message size, after serialze but before compress.
const int kMaxRequestSize = 32 * 1024 * 1024;

// using namespace common;

class RpcClient;

// RpcChannel is a handle class.  During construction, it creates real
// channel object according to the server name prefix.
class RpcChannel : public RpcChannelInterface
{
    friend class RpcChannelTest;

public:
    RpcChannel(RpcClient* rpc_client,
               const std::string& server_name,
               common::CredentialGenerator* credential_generator = NULL,
               const RpcChannelOptions& options = RpcChannelOptions());
    virtual ~RpcChannel() {}

    // Implements the google::protobuf::RpcChannel interface.  If the
    // done is NULL, it's a synchronous call, or it's an asynchronous
    // and uses the callback inform the completion (or failure).  It's
    // only called by user's thread.
    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done);

    // TODO(dphuang): Give GetChannelStatus a more preferrable name.
    virtual ChannelStatus GetChannelStatus() const;

private:
    // m_channel pointes to the real channel implementation.
    scoped_ptr<RpcChannelInterface> m_channel;
};

} // namespace poppy

#endif // POPPY_RPC_CHANNEL_H
