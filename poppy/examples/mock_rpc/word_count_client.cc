// Copyright 2011 Tencent Inc.
// Author: Yi Wang <yiwang@tencent.com>
//
#include "poppy/examples/mock_rpc/word_count_client.h"
#include "thirdparty/glog/logging.h"

namespace poppy {
namespace examples {
namespace mock_rpc {

// This is an example RPC client of service WordCountService.
void WordCounter::Count(const std::string& text, int* count,
                        google::protobuf::Closure* closure) {
    WordCountRequest* request = new WordCountRequest;
    request->set_text(text);
    WordCountResponse* response = new WordCountResponse;
    poppy::RpcController* controller = new poppy::RpcController;
    if (closure == NULL) {
        m_stub.Count(controller, request, response, NULL);
        *count = GetCountValue(controller, response);
        delete controller;
        delete request;
        delete response;
    } else {
        google::protobuf::Closure* count_callback = NewClosure(this,
                                                               &WordCounter::CountCallback,
                                                               controller,
                                                               request,
                                                               response,
                                                               count,
                                                               closure);

        m_stub.Count(controller, request, response, count_callback);
    }
}

void WordCounter::CountCallback(poppy::RpcController* controller,
                                WordCountRequest* request,
                                WordCountResponse* response,
                                int* count,
                                google::protobuf::Closure* closure) {
    *count = GetCountValue(controller, response);
    delete controller;
    delete request;
    delete response;
    closure->Run();
}

int WordCounter::GetCountValue(const poppy::RpcController* controller,
                               WordCountResponse* response) const {
    int count = 0;
    if (controller->Failed()) {
        count = -1;
    } else {
        count = response->count();
    }
    return count;
}

}  // namespace mock_rpc
}  // namespace examples
}  // namespace poppy


