// Copyright 2011, Tencent Inc.
// Author: Fang Liu <kenfangliu@tencent.com>
//         Hangjun Ye <hansye@tencent.com>
//         Yi Wang <yiwang@tencent.com>
//
// Here defines the RpcMockChannel.  For more information about
// mocking an RPC service, please refer to rpc_mock.h.
//
// In Poppy, and other RPC implementations for Google Protobuf,
// invocations to remote services are realized by invoking a service
// stub, whose constructor requires an RPC channel object.  The
// channel object, in production code, should be an implementation of
// the interface google::protobuf::RpcChannel, whose CallMethod method
// sends requests and receives responses over the network; whereas, in
// unit tests, it is should be RpcModelChannel, whose CallMethod is a
// mock method.  Actions of this mock method can be specified in test
// code via gmock.  Usually, it pretends to invoke a remote procedure,
// but indeed directly returns something as you specified.
//
#ifndef POPPY_RPC_MOCK_CHANNEL_H_
#define POPPY_RPC_MOCK_CHANNEL_H_

#include <string>
#include <vector>

#include "poppy/rpc_channel_interface.h"
#include "poppy/rpc_client.h"
#include "poppy/rpc_controller.h"

#include "thirdparty/glog/logging.h"
#include "thirdparty/protobuf/descriptor.h"

namespace poppy {

class RpcMockChannel : public RpcChannelInterface {
public:
    RpcMockChannel();
    virtual ~RpcMockChannel() {}

    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done);

    virtual ChannelStatus GetChannelStatus() const {
        return ChannelStatus_Healthy;
    }
};

}  // namespace poppy

#endif  // POPPY_RPC_MOCK_CHANNEL_H_
