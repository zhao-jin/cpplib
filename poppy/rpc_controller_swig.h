// Copyright 2011, Tencent Inc.
// Author: Eric Liu <ericliu@tencent.com>
//
// The implementation of rpc controller for swig.

#ifndef POPPY_RPC_CONTROLLER_SWIG_H
#define POPPY_RPC_CONTROLLER_SWIG_H

#include <string>

#include "poppy/rpc_controller.h"

namespace poppy {

class RpcControllerSwig : public RpcController
{
public:
    RpcControllerSwig() {}
    ~RpcControllerSwig() {}

    using RpcController::Timeout;
    using RpcController::set_method_full_name;

    void SetFailed(int error_code, const std::string& reason = "")
    {
        RpcController::SetFailed(error_code, reason);
    }
    std::string RemoteAddressString() const
    {
        return RemoteAddress().ToString();
    }

private:
    void SetFailed(const std::string& reason) {}
};

} // namespace poppy

#endif // POPPY_RPC_CONTROLLER_SWIG_H
