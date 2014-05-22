// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: CHEN Feng <phongchen@tencent.com>
// Created: 12/21/11
// Description:

#undef __DEPRECATED // to test deprecated interface
#include "poppy/rpc_client.h"
#include "common/netframe/netframe.h"
#include "thirdparty/gtest/gtest.h"

namespace poppy {

TEST(RpcClient, DefaultCtor)
{
    RpcClient client;
}

TEST(RpcClient, CtorWithNetframe)
{
    netframe::NetFrame netframe;
    RpcClient client1(&netframe);
    RpcClient client2(&netframe, false);
}

TEST(RpcClient, CtorWithThreadNumber)
{
    RpcClient client2(2);
}

TEST(RpcClient, CtorWithOptions)
{
    netframe::NetFrame netframe;
    RpcClientOptions options;
    RpcClient client1(options);
    RpcClient client2(options, NULL);
    RpcClient client3(options, &netframe);

    options.set_own_netframe(true);
    netframe::NetFrame* pnetframe = new netframe::NetFrame();
    RpcClient client4(options, pnetframe);
}

TEST(RpcClient, Shutdown)
{
    RpcClient client;
    client.Shutdown();
    client.Shutdown(); // test shutdown twice
}

} // namespace poppy
