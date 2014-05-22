// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
//         Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef POPPY_RPC_HTTP_CHANNEL_H
#define POPPY_RPC_HTTP_CHANNEL_H

#include <string>
#include <vector>
#include "common/base/stdext/shared_ptr.h"
#include "poppy/rpc_channel_impl.h"
#include "poppy/rpc_channel_interface.h"
#include "poppy/rpc_channel_option_info.pb.h"
#include "poppy/rpc_client_impl.h"

namespace common {
class CredentialGenerator;
} // namespace common

namespace poppy {

// using namespace common;

// The client rpc channel.
class RpcHttpChannel : public RpcChannelInterface
{
public:
    RpcHttpChannel(const stdext::shared_ptr<RpcClientImpl>& rpc_client_impl,
                   const std::string& name,
                   common::CredentialGenerator* credential_generator = NULL,
                   const RpcChannelOptions& options = RpcChannelOptions());
    virtual ~RpcHttpChannel();

    // Implements the google::protobuf::RpcChannel interface.
    // If the done is NULL, it's a synchronous call, or it's an asynchronous
    // and uses the callback inform the completion (or failure).
    // It's only called by user's thread.
    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done);

    static void OnResponseReceived(stdext::weak_ptr<RpcClientImpl> rpc_client_impl,
                                   RpcController* controller,
                                   google::protobuf::Message* response,
                                   google::protobuf::Closure* done,
                                   const StringPiece* message_data);

    virtual ChannelStatus GetChannelStatus() const;

protected:
    stdext::shared_ptr<RpcChannelImpl> m_channel_impl;
    stdext::weak_ptr<RpcClientImpl> m_rpc_client_impl;
};

}  // namespace poppy

#endif // POPPY_RPC_HTTP_CHANNEL_H
