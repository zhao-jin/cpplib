// Copyright (c) 2013, Tencent Inc.
// All rights reserved.
//
// Author: CHEN Feng <phongchen@tencent.com>
// Created: 2013-07-31

#include "poppy/rpc_error_code.h"
#include "thirdparty/gtest/gtest.h"

namespace poppy {

TEST(RpcErrorCode, ToString)
{
    EXPECT_EQ("RPC_SUCCESS", RpcErrorCodeToString(RPC_SUCCESS));
    EXPECT_EQ("", RpcErrorCodeToString(-1));
}

} // namespace poppy

