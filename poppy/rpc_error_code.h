// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: CHEN Feng <phongchen@tencent.com>
// Created: 12/21/11
// Description: rpc error code relatived interfaces

#ifndef POPPY_RPC_ERROR_CODE_H
#define POPPY_RPC_ERROR_CODE_H
#pragma once

#include <string>
#include "poppy/rpc_error_code_info.pb.h"

namespace poppy {

// convert rpc error code to human readable string
const std::string& RpcErrorCodeToString(int error_code);

} // namespace poppy

#endif // POPPY_RPC_ERROR_CODE_H
