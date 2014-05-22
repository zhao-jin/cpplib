// Copyright 2011, Tencent Inc.
// Author: Yi Wang <yiwang@tencent.com>

#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"
#include "common/base/string/algorithm.h"
#include "poppy/rpc_http_channel.h"

namespace poppy {

// using namespace common;

// The server name prefix must be either a substring within a pair of
// '/' and '/' indicating a channel type, or an empty string
// corresponding to the default channel.
static std::string GetServerNamePrefix(const std::string& server_name) {
    if (server_name[0] == '/') {
        size_t second_slash = server_name.find('/', 1);
        if (second_slash != std::string::npos) {
            return server_name.substr(0, second_slash + 1);
        }
    }
    return std::string("");
}

RpcChannel::RpcChannel(
        RpcClient* rpc_client,
        const std::string& server_name,
        common::CredentialGenerator* credential_generator,
        const RpcChannelOptions& options) {
    CHECK(!server_name.empty()) << "Empty channel address";
    m_channel.reset(POPPY_CREATE_RPC_CHANNEL_IMPL(GetServerNamePrefix(
            server_name)));

    if (m_channel.get() == NULL) {
        m_channel.reset(new RpcHttpChannel(rpc_client->impl(),
                                           server_name,
                                           credential_generator, options));
    }
}

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) {
    m_channel->CallMethod(method, controller, request, response, done);
}

ChannelStatus RpcChannel::GetChannelStatus() const {
    return m_channel->GetChannelStatus();
}

}  // namespace poppy
