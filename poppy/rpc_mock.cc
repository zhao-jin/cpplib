// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author:  Cui Jianwei <jianweicui@tencent.com>
// Created: 2012-02-07

#include "poppy/rpc_mock.h"

namespace poppy {

void RpcControllerSetFailed(int error_code, RpcController* rpc_controller)
{
    rpc_controller->SetFailed(error_code);
}

void RunCallbackInMockChannel(::google::protobuf::Closure* callback)
{
    if (callback != NULL) {
        RpcMockChannelImpl& mock_channel_impl = Singleton<RpcMockChannelImpl>::Instance();
        mock_channel_impl.AddCallbackToTimer(callback);
    }
}

void RunCallbackInMockChannelAfterInterval(::google::protobuf::Closure* callback, int64_t interval)
{
    if (callback == NULL) {
        ThisThread::Sleep(interval);
    } else {
        RpcMockChannelImpl& mock_channel_impl = Singleton<RpcMockChannelImpl>::Instance();
        mock_channel_impl.AddCallbackToTimer(callback, interval);
    }
}

} // namespace poppy
