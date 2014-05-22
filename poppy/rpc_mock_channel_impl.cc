// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author:  Cui Jianwei <jianweicui@tencent.com>
// Created: 2012-02-07

#include "poppy/rpc_mock_channel_impl.h"

namespace poppy {

void RpcMockChannelImpl::AddCallbackToTimer(::google::protobuf::Closure* closure, int64_t wait_time)
{
    TimerManager::CallbackClosure* timer_callback =
        NewClosure(this, &RpcMockChannelImpl::RunCallback, closure);
    m_timer_manager.AddOneshotTimer(wait_time, timer_callback);
}

void RpcMockChannelImpl::RunCallback(google::protobuf::Closure* closure, uint64_t timer_id) {
    closure->Run();
}

} // namespace poppy
