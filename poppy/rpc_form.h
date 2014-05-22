// Copyright 2011, Tencent Inc.
// Author: Eric Liu <ericliu@tencent.com>

#ifndef POPPY_RPC_FORM_H
#define POPPY_RPC_FORM_H

#include "common/base/closure.h"

// namespace common {
class HttpRequest;
class HttpResponse;
// } // namespace common

namespace poppy {

// using namespace common;

class RpcServerHandler;

void HandleFormRequest(const HttpRequest* http_request,
                       HttpResponse* http_response,
                       Closure<void>* done);

} // namespace poppy

#endif // POPPY_RPC_FORM_H
