// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: CHEN Feng <phongchen@tencent.com>
// Created: 12/21/11
// Description: rpc error code implementation

#include "poppy/rpc_error_code.h"

namespace poppy {

const std::string& RpcErrorCodeToString(int error_code)
{
    return RpcErrorCode_Name(static_cast<RpcErrorCode>(error_code));
}

} // namespace poppy
