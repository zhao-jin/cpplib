// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/rpc_http_channel.h"

#include "common/system/concurrency/thread.h"
#include "glog/logging.h"
#include "poppy/rpc_controller.h"
#include "protobuf/descriptor.h"

namespace poppy {

RpcHttpChannel::RpcHttpChannel(
    const stdext::shared_ptr<RpcClientImpl>& rpc_client_impl,
    const std::string& name,
    common::CredentialGenerator* credential_generator,
    const RpcChannelOptions& options) :
    m_channel_impl(rpc_client_impl->GetChannelImpl(name,
                                                   credential_generator,
                                                   options)),
    m_rpc_client_impl(rpc_client_impl)
{
}

RpcHttpChannel::~RpcHttpChannel()
{
    stdext::shared_ptr<RpcClientImpl> rpc_client = m_rpc_client_impl.lock();
    if (rpc_client) {
        m_channel_impl.reset();
        rpc_client->ReleaseChannelImpl();
    }
}

void RpcHttpChannel::CallMethod(
        const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    RpcController* rpc_controller =
            static_cast<RpcController*>(controller);
    rpc_controller->InternalReset();
    rpc_controller->set_sync(done == NULL);
    rpc_controller->FillFromMethodDescriptor(method);
    Closure<void, const StringPiece*>* callback = NewClosure(
            &RpcHttpChannel::OnResponseReceived,
            m_rpc_client_impl, rpc_controller, response, done);
    m_channel_impl->CallMethod(rpc_controller, request, callback);
}

void RpcHttpChannel::OnResponseReceived(stdext::weak_ptr<RpcClientImpl> rpc_client_impl,
                                        RpcController* rpc_controller,
                                        google::protobuf::Message* response,
                                        google::protobuf::Closure* done,
                                        const StringPiece* message_data)
{
    stdext::shared_ptr<RpcClientImpl> rpc_client = rpc_client_impl.lock();
    if (rpc_client == NULL)
        return;

    if (!rpc_controller->Failed() &&
        !response->ParseFromArray(message_data->data(),
                                  message_data->size())) {
#ifdef NDEBUG
        LOG(ERROR) << "Failed to parse the response message, it is missing some required fields";
#else
        LOG(ERROR) << "Failed to parse the response message, it is missing required fields: "
            << response->InitializationErrorString();
#endif
        rpc_controller->SetFailed(RPC_ERROR_PARSE_RESPONES_MESSAGE);
    }

    rpc_controller->set_in_use(false);

    if (!rpc_controller->is_sync()) {
        ThreadPool* thread_pool = rpc_client->GetThreadPool();
        thread_pool->AddTask(done);
    }
}

ChannelStatus RpcHttpChannel::GetChannelStatus() const {
    return m_channel_impl->GetChannelStatus();
}

} // namespace poppy
