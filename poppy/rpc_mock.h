// Copyright 2011, Tencent Inc.
// Author: Fang Liu <kenfangliu@tencent.com>
//         Hangjun Ye <hansye@tencent.com>
//         Yi Wang <yiwang@tencent.com>
//
// Here we extends gmock to support mocking of RPC services.
//
// This mechanism, in general, mocks the RPC channel, instead of each
// RPC services.  This is because client invocations to any service
// goes through RpcChannel::CallMethod, and mocking it allows gmock to
// monitor any interactions between the client and the server.  For
// more details, please refer to rpc_mock_channel.h.
//
// HOW TO WRITE CLIENT CODE:
//
// You write Poppy client code regardless it is to talk with a real
// server or a mock server.  The only difference is that a mock server
// has name prefix "mock://", whereas a real server might be referred
// to as <ip>:<port> or <address>:<port>, and <address> might be a
// Zookeeper URL with prefix "zk://".
//
// HOWT TO WRITE TEST CODE:
//
// This file extends gmock in supporting unittest of RPC client code.
// You can specify expected behavior of RPC services using EXPECT_RPC.
// Say, we have an RPC server WordCountService, whose method
// Count(WordCountRequest, WordCountResponse) counts the number of
// words in WordCountRequest::text() and returns result in
// WordCountResponse::count().
//
// Suppose you want to define the behavior that "Steve Jobs" is a
// single word:
//
// WordCountRequest kReqOne;
// kReqOne.set_text("Steve Jobs");
//
// WordCountResponse kRespOne;
// kRespOne.set_count(1);
//
// EXPECT_RPC("examples.mock_rpc.WordCountService.Count",
//            kReqOne)
//     .WillOnce(RPC_RESPOND(kRespOne));
//
// Then, create and invoke RPC clients as you did in production code:
//
//   WordCountClient client(FLAGS_server_name);  // which invokes NewRpcChannel
//   EXPECT_EQ(1, client.Count(kReqOne));
//
// You can also expect a fail of an RPC invocation through action
// RPC_FAIL_CODE (if you can provide an error code) or RPC_FAIL_REASON
// (if you can provide an error string):
//
// EXPECT_RPC("examples.mock_rpc.WordCountService.Count",
//            kReqNil)
//     .WillOnce(RPC_FAIL_REASON("Unkown text encoding"));
//
// For more examples, please refer to poppy/exmaples/mock_rpc/mock_rpc_test.cc.

#ifndef POPPY_RPC_MOCK_H
#define POPPY_RPC_MOCK_H

#include <string>

#include "common/base/singleton.h"
#include "common/protobuf_extensions/field_predicates.h"
#include "common/protobuf_extensions/find_field.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_controller.h"
#include "poppy/rpc_mock_channel.h"
#include "poppy/rpc_mock_channel_impl.h"
#include "thirdparty/gmock/gmock.h"

namespace poppy {

//
// Customized matchers.
//

// Judge whether the request proto message matches a given message.
MATCHER_P(ProtoMessageEq, rhs_msg, " has content ") {
    std::string lhs_enc;
    std::string rhs_enc;
    arg->SerializeToString(&lhs_enc);
    rhs_msg.SerializeToString(&rhs_enc);
    // TODO(yiwang): comparing two proto messages by their
    // serializations can NOT guarentee de-duplication.  We need to do
    // recursive comparison of two messages using protobuf's reflection
    // mechanism.
    return lhs_enc == rhs_enc;
}

// Judge whether the the request proto message has given field.  For
// details about how field_name indicate a field, please refer to file
// comment of protobuf_extension.h.
//
MATCHER_P(HasFieldValue, field_name, " has field value ") {
    namespace pb = google::protobuf;
    namespace pbext = google::protobuf::extension::tencent;
    const pb::Message* closure = NULL;
    const pb::FieldDescriptor* field_desc = NULL;
    return pbext::FindProtoMessageField(*arg, field_name, &closure, &field_desc);
}

// Judge whether the specified singular field value, i.e.,
// required/optional and numeric/string, in the request proto message
// equals to the given value.
//
MATCHER_P2(SingularFieldEq, field_name, field_value, " field value is ") {
    namespace pb = google::protobuf;
    namespace pbext = google::protobuf::extension::tencent;
    const pb::Message* closure = NULL;
    const pb::FieldDescriptor* field_desc = NULL;
    return pbext::FindProtoMessageField(*arg, field_name, &closure, &field_desc)
            && pbext::IsSingularValueField(field_desc)
            && pbext::SingularFieldValueEq(*closure, field_desc, field_value);
}

// Judge whether the size of speicfied field, which must be a repeated
// field, matches the given size.
//
MATCHER_P2(RepeatedFieldSizeEq, field_name, field_size,
           " is a repeated field with size ") {
    namespace pb = google::protobuf;
    namespace pbext = google::protobuf::extension::tencent;
    const pb::Message* closure = NULL;
    const pb::FieldDescriptor* field_desc = NULL;
    return pbext::FindProtoMessageField(*arg, field_name, &closure, &field_desc)
            && pbext::IsRepeatedValueField(field_desc)
            && pbext::RepeatedFieldSizeEq(*closure, field_desc, field_size);
}

// Judge whether the value of specified element in the repeated field
// matches the given value.
///
MATCHER_P3(RepeatedFieldElementEq, field_name, element_index, value,
           " element in repeated field is ") {
    namespace pb = google::protobuf;
    namespace pbext = ::google::protobuf::extension::tencent;
    const pb::Message* closure = NULL;
    const pb::FieldDescriptor* field_desc = NULL;
    return pbext::FindProtoMessageField(*arg, field_name, &closure, &field_desc)
            && pbext::IsRepeatedValueField(field_desc)
            && pbext::SingularElementValueEq(*closure, field_desc,
                                             element_index, value);
}

//
// Customized actions.
//

// The function is used to set error_code of RpcController.
// Becareful, users are not allowed to call this function.
void RpcControllerSetFailed(int error_code, RpcController* rpc_controller);

// The following two functions will run the users' callback in mock_channel.
// Becareful, usres are not allowd to call this function.
void RunCallbackInMockChannel(::google::protobuf::Closure* callback);
void RunCallbackInMockChannelAfterInterval(::google::protobuf::Closure* callback, int64_t interval);

ACTION_P(Respond, value) {
    arg3->CopyFrom(value);
    RunCallbackInMockChannel(arg4);
}

ACTION_P(FailCode, error_code) {
    RpcControllerSetFailed(error_code, static_cast<poppy::RpcController *>(arg1));
    if (error_code == RPC_ERROR_REQUEST_TIMEOUT) {
        // if the error_code is RPC_ERROR_REQUEST_TIMEOUT, the callback will be run
        // after rpc_controller->Timeout() when it is not NULL; otherwise, the rpc call
        // will return after rpc_controller->Timeout()
        int64_t timeout = (static_cast< ::poppy::RpcController *>(arg1))->Timeout();
        RunCallbackInMockChannelAfterInterval(arg4, timeout);
    } else {
        RunCallbackInMockChannel(arg4);
    }
}

ACTION_P(FailReason, reason) {
    arg1->SetFailed(reason);
    RunCallbackInMockChannel(arg4);
}

// TODO(yiwang): We need more actions.  For example, to invoke the
// callback Done() after some time, in order to simulate asynchronous
// calls.

}  // namespace poppy

//
// EXPECT_RPC works with RPC invocations like EXPECT_CALL works with
// local invocations.
//

#define EXPECT_RPC(rpc_method_name, request_matcher)                    \
    EXPECT_CALL(                                                        \
            Singleton< ::poppy::RpcMockChannelImpl>::Instance(),                  \
            CallMethod(                                                 \
                    ::testing::Property(                                \
                            &google::protobuf::MethodDescriptor::full_name, \
                            rpc_method_name),                           \
                    ::testing::NotNull(),                               \
                    request_matcher,                                    \
                    ::testing::_,                                       \
                    ::testing::_))

#endif  // POPPY_RPC_MOCK_H
