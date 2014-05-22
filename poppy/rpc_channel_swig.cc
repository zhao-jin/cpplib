// Copyright 2011, Tencent Inc.
// Author: Eric Liu <ericliu@tencent.com>

#include "poppy/rpc_channel_swig.h"

#include "common/system/concurrency/thread.h"
#include "poppy/rpc_client_impl.h"

namespace poppy {

// using namespace common;

void ResponseHandler::InnerRun(const std::string* response_data_ptr)
{
    Run(*response_data_ptr);
    delete response_data_ptr;
    delete this;
}

RpcChannelSwig::RpcChannelSwig(RpcClient* rpc_client,
                               const std::string& name) :
    m_channel_impl(rpc_client->impl()->GetChannelImpl(name)),
    m_rpc_client(rpc_client)
{
}

RpcChannelSwig::~RpcChannelSwig()
{
}

std::string RpcChannelSwig::RawCallMethod(RpcController* rpc_controller,
        const std::string& request_data,
        ResponseHandler* handler)
{
    rpc_controller->InternalReset();
    rpc_controller->set_sync(handler == NULL);
    std::string* response_data_ptr;
    std::string response_data;

    if (rpc_controller->is_sync()) {
        response_data_ptr = &response_data;
    } else {
        response_data_ptr = new std::string;
    }

    Closure<void, const StringPiece*>* done = NewClosure(
            &RpcChannelSwig::OnResponseReceived, m_rpc_client,
            rpc_controller, response_data_ptr, handler);
    m_channel_impl->RawCallMethod(rpc_controller, &request_data, done);

    if (rpc_controller->is_sync()) {
        return response_data;
    } else {
        return std::string();
    }
}

void RpcChannelSwig::OnResponseReceived(RpcClient* rpc_client,
                                        RpcController* rpc_controller,
                                        std::string* response_data_ptr,
                                        ResponseHandler* handler,
                                        const StringPiece* message_data)
{
    if (!rpc_controller->Failed()) {
        if (message_data) {
            response_data_ptr->assign(message_data->data(), message_data->size());
        }
    }

    rpc_controller->set_in_use(false);

    if (!rpc_controller->is_sync()) {
        ThreadPool* thread_pool = rpc_client->impl()->GetThreadPool();
        thread_pool->AddTask(NewClosure(handler, &ResponseHandler::InnerRun,
                                        static_cast<const std::string*>(response_data_ptr)));
    }
}

} // namespace poppy
