// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
//         Yi Wang <yiwang@tencent.com>
//
// This file defines the RpcMockChannelImpl and related operations.
// In the address process of a process, there could be multiple
// RpcMockChannel instances, but only one RpcMockChannelImpl instance.
// The macro EXPECT_RPC puts all expectations to this singleton mock
// implementation, whereas the second parameter of EXPECT_RPC,
// full_method_name, is a full name consisting the service name and
// method name and is able to distinguish invocations to services on
// multiple channels.
//
#ifndef POPPY_RPC_MOCK_CHANNEL_IMPL_H_
#define POPPY_RPC_MOCK_CHANNEL_IMPL_H_

#include "common/base/closure.h"
#include "common/system/timer/timer_manager.h"
#include "thirdparty/gmock/gmock.h"
#include "thirdparty/protobuf/service.h"

namespace poppy {

// As the real mocked channel, RpcMockChannelImpl implements
// google::protobuf::RpcChannel.  It also inherits from
// RefCountedBase<RpcMockChannelImpl>, which adds a reference count
// value and related operations.  This makes it possible to refer to
// RpcMockChannelImpl object from a shared_ptr.
//
class RpcMockChannelImpl : public google::protobuf::RpcChannel
{
public:
    RpcMockChannelImpl() {}

    virtual ~RpcMockChannelImpl() {}

    MOCK_METHOD5(CallMethod,
                 void(const google::protobuf::MethodDescriptor* /*method*/,
                      google::protobuf::RpcController* /*controller*/,
                      const google::protobuf::Message* /*request*/,
                      google::protobuf::Message* /*response*/,
                      google::protobuf::Closure* /*done*/));

    // AddCallbackToTimer will run closure after wait_time. Don't call this function
    // with closure=NULL because it won't check this when running closure.
    void AddCallbackToTimer(google::protobuf::Closure* closure,
                            int64_t wait_time = kDefaultCallbackWaitTime);

private:
    /// default timer interval after which user's callback will be call.
    /// The user's callback will be called after kDefaultCallbackWaitTime default if
    /// wait_time is not set in AddCallbackToTimer, which aims to avoid the callback
    /// always be called before rpc call return to make simulate async call better.
    /// Maybe there are better methods to achieve this goal?
    static const int64_t kDefaultCallbackWaitTime = 1;

    void RunCallback(google::protobuf::Closure* closure, uint64_t timer_id);

private:
    TimerManager m_timer_manager; ///< 调用用户回调函数的定时器
};
}  // namespace poppy

#endif  // POPPY_RPC_MOCK_CHANNEL_IMPL_H_

