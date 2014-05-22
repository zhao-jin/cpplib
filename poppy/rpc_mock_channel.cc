// Copyright 2011, Tencent Inc.
// Author: Yi Wang <yiwang@tencent.com>
//
#include "poppy/rpc_mock_channel.h"

#include "common/base/module.h"
#include "common/base/singleton.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_mock_channel_impl.h"
#include "thirdparty/gtest/gtest.h"

namespace poppy {

// We register class RpcMockChannel using class registry mechanism, so
// that libpoppy does not depend on libpoppy_mock, although an
// RpcChannel instance might creates an RpcMockChannel instance, if
// the provided server name starts with kMockServerNamePrefix.  This
// is valuable as we do not want production code relying on mock code.
const std::string kMockServerNamePrefix("/mock/");
POPPY_REGISTER_RPC_CHANNEL_IMPL(kMockServerNamePrefix, RpcMockChannel);

// This listener invokes Mock::VerifyAndClearExpectations of
// google-mock to clear expectations associated with the mock channel
// singleton at the end of any test.  This is important since the next
// test would likely set its new expectations.
class RpcMockTestEventListener : public ::testing::EmptyTestEventListener {
public:
    virtual void OnTestEnd(const ::testing::TestInfo& test_info) {
        ::testing::Mock::VerifyAndClearExpectations(
                &Singleton<RpcMockChannelImpl>::Instance());
    }
};

// This global variable indicates the loading of the
// rpc_mock_test_event_listener_module, which appends
// RpcMockTestEventListener to google-mock runtime.
bool g_rpc_mock_test_event_listener_activated = false;

RpcMockChannel::RpcMockChannel() {
    if (!g_rpc_mock_test_event_listener_activated) {
        LOG(FATAL) << "Please make sure that the main function of your "
                   << "unittest invokes InitAllModules defined in "
                   << "//common/base/module.h";
    }
}

void RpcMockChannel::CallMethod(
        const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done) {
    Singleton<RpcMockChannelImpl>::Instance().CallMethod(
            method, controller, request, response, done);
}

}  // namespace poppy

DEFINE_MODULE(rpc_mock_test_event_listener_module) {
    ::testing::TestEventListeners& listeners =
            ::testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new poppy::RpcMockTestEventListener);
    poppy::g_rpc_mock_test_event_listener_activated = true;
    return true;
}
