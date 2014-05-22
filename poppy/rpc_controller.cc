// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: CHEN Feng <phongchen@tencent.com>
// Created: 12/21/11
// Description: RpcController implementation

#include "poppy/rpc_controller.h"
#include "common/base/string/concat.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/protobuf/descriptor.h"

namespace poppy {

RpcController::RpcController()
    : m_time(0),
      m_connection_id(-1),
      m_sequence_id(-1),
      m_error_code(RPC_SUCCESS),
      m_canceled(false),
      m_fail_immediately(false),
      m_in_use(false),
      m_sync(false)
{
    m_user_options.timeout = 0;
    m_user_options.request_compress_type = CompressTypeAuto;
    m_user_options.response_compress_type = CompressTypeAuto;

    m_auto_options.timeout = 0;
    m_auto_options.request_compress_type = CompressTypeNone;
    m_auto_options.response_compress_type = CompressTypeNone;
}

RpcController::~RpcController()
{
    // For debug purpose
    m_time = -m_time;
    m_connection_id = -m_connection_id;
    m_sequence_id = -m_sequence_id;
    m_error_code = -1;
}

void RpcController::InternalReset()
{
    m_time = 0;
    m_connection_id = -1;
    m_sequence_id = -1;
    m_error_code = RPC_SUCCESS;
    m_sync = false;
    m_canceled = false;
    m_reason.clear();
    m_cancel_listeners.clear();
    m_credential.clear();
    m_user.clear();
    m_role.clear();
    m_auto_options.timeout = 0;
    m_auto_options.request_compress_type = CompressTypeNone;
    m_auto_options.response_compress_type = CompressTypeNone;
}

void RpcController::Reset()
{
    CHECK(!in_use()) << "You can't reset a controller when is it in use";

    InternalReset();
    m_user_options.timeout = 0;
    m_user_options.request_compress_type = CompressTypeAuto;
    m_user_options.response_compress_type = CompressTypeAuto;
}

std::string RpcController::ErrorText() const
{
    if (m_reason.empty()) {
        return RpcErrorCodeToString(m_error_code);
    } else {
        return RpcErrorCodeToString(m_error_code) + ": " + m_reason;
    }
}

// Actually the current implementatoin is NOT correct, even several open
// source protobuf rpc implementations are doing this.
// The semantics of this function should be the client side send a request
// to server side to cancel an ongoing RPC call, all listeners on server
// side would be notified.
// TODO(hansye): re-consider the implementation.
void RpcController::StartCancel()
{
    m_canceled = true;

    for (std::list<google::protobuf::Closure*>::const_iterator i =
         m_cancel_listeners.begin();
         i != m_cancel_listeners.end();
         ++i) {
        (*i)->Run();
    }

    m_cancel_listeners.clear();
}

void RpcController::FillFromMethodDescriptor(
    const google::protobuf::MethodDescriptor* method)
{
    if (m_user_options.timeout <= 0) {
        int64_t timeout_in_ms = method->options().HasExtension(method_timeout) ?
                method->options().GetExtension(method_timeout) :
                method->service()->options().GetExtension(service_timeout);

        if (timeout_in_ms <= 0) {
            // Just a protection, it shouldn't happen.
            timeout_in_ms = 1;
        }

        m_auto_options.timeout = timeout_in_ms;
    }

    m_method_full_name = method->full_name();

    if (m_user_options.request_compress_type == CompressTypeAuto) {
        CompressType request_compress_type = CompressTypeNone;
        if (method->options().HasExtension(poppy::request_compress_type)) {
            request_compress_type =
                method->options().GetExtension(poppy::request_compress_type);
        }
        m_auto_options.request_compress_type = request_compress_type;
    }

    if (m_user_options.response_compress_type == CompressTypeAuto) {
        CompressType response_compress_type = CompressTypeNone;
        if (method->options().HasExtension(poppy::response_compress_type)) {
            response_compress_type =
                method->options().GetExtension(poppy::response_compress_type);
        }
        m_auto_options.response_compress_type = response_compress_type;
    }
}

std::string RpcController::Dump() const
{
    std::string result;

    StringAppend(
        &result,
        "RpcController: "
        "method=", m_method_full_name, ", "
        "sequence_id=", m_sequence_id, ", "
        "remote_address=", RemoteAddress().ToString(), ", "
        "time=", m_time
    );

    if (Failed()) {
        StringAppend(&result, ", error=", ErrorCode(), "(", ErrorText(), ")");
    }

    return result;
}

size_t RpcController::MemoryUsage() const
{
    return sizeof(*this) +
        m_method_full_name.capacity() +
        m_reason.capacity() +
        m_credential.capacity() +
        m_user.capacity() +
        m_role.capacity() +
        m_cancel_listeners.size() * sizeof(google::protobuf::Closure);
}

} // namespace poppy
