// Copyright 2011 Tencent Inc.
// Author: Yi Wang <yiwang@tencent.com>
//
// This is a simple syncrhonous client for an example Poppy service,
// WordCountService.  This client serves as an example of using Poppy
// mock framework in unittests.
//
#ifndef POPPY_EXAMPLES_MOCK_RPC_WORD_COUNT_CLIENT_H_
#define POPPY_EXAMPLES_MOCK_RPC_WORD_COUNT_CLIENT_H_

#include <string>
#include "common/base/closure.h"
#include "common/base/scoped_ptr.h"
#include "poppy/examples/mock_rpc/word_count_service.pb.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"

namespace poppy {
namespace examples {
namespace mock_rpc {

// This is an example RPC client of service WordCountService.
class WordCounter {
public:
    explicit WordCounter(const std::string& server_name)
            : m_channel(new poppy::RpcChannel(&m_client, server_name)),
              m_stub(m_channel.get()) {}

    void Count(const std::string& text, int* count, google::protobuf::Closure* closure);

private:
    void CountCallback(poppy::RpcController* controller,
                       WordCountRequest* request,
                       WordCountResponse* response,
                       int* count,
                       google::protobuf::Closure* closure);

    int GetCountValue(const poppy::RpcController* controller,
                      WordCountResponse* response) const;

    poppy::RpcClient m_client;
    scoped_ptr<google::protobuf::RpcChannel> m_channel;
    WordCountService::Stub m_stub;
};

}  // namespace mock_rpc
}  // namespace examples
}  // namespace poppy

#endif  // POPPY_EXAMPLES_MOCK_RPC_WORD_COUNT_CLIENT_H_
