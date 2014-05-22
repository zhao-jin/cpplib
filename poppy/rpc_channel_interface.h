// Copyright 2011, Tencent Inc.
// Author: Yi Wang <yiwang@tencent.com>

#ifndef POPPY_RPC_CHANNEL_INTERFACE_H_
#define POPPY_RPC_CHANNEL_INTERFACE_H_

#include <string>
#include <vector>
#include "common/base/class_register.h"
#include "thirdparty/protobuf/service.h"

namespace poppy {

// Healthy status of channel
enum ChannelStatus {
    ChannelStatus_Unknown = 0,
    ChannelStatus_Unavailable,
    ChannelStatus_Healthy,
    ChannelStatus_Unconnectable,
    ChannelStatus_NoAuth,
    ChannelStatus_Shutdown
};

// The channel of Poppy extends that of Google's open source version.
class RpcChannelInterface : public google::protobuf::RpcChannel {
public:
    virtual ~RpcChannelInterface() {}

    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) = 0;

    virtual ChannelStatus GetChannelStatus() const = 0;
};

}  // namespace poppy

// Here we define a class registery of RPC channel implementations.
// According to the server_name_prefix, an RpcChannel instance creates
// underlying channel instance.
CLASS_REGISTER_DEFINE_REGISTRY(rpc_channel_registery,           \
                               poppy::RpcChannelInterface);

#define POPPY_REGISTER_RPC_CHANNEL_IMPL(server_name_prefix,     \
                                        channel_impl)           \
    CLASS_REGISTER_OBJECT_CREATOR(                              \
            rpc_channel_registery, RpcChannelInterface,         \
            server_name_prefix, channel_impl)

#define POPPY_CREATE_RPC_CHANNEL_IMPL(server_name_prefix)       \
    CLASS_REGISTER_CREATE_OBJECT(rpc_channel_registery,         \
                                 server_name_prefix)

#endif // POPPY_RPC_CHANNEL_INTERFACE_H_
