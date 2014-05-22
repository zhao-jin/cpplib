// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: CHEN Feng <phongchen@tencent.com>
// Created: 12/21/11
// Description: RpcChannel test

#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"
#include "thirdparty/gtest/gtest.h"

namespace poppy {

TEST(RpcChannel, Ctor)
{
    RpcClient client;
    RpcChannel channel(&client, "127.0.0.1:10000");
}

TEST(RpcChannel, CtorWithCreditial)
{
    RpcClient client;
    RpcChannel channel(&client, "127.0.0.1:10000", NULL);
}

TEST(RpcChannel, CtorWithOptions)
{
    RpcClient client;
    RpcChannelOptions options;
    RpcChannel channel(&client, "127.0.0.1:10000", NULL, options);
}

} // namespace poppy
