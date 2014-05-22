// Copyright (c) 2012, Tencent Inc.
// All rights reserved.
//
// Author: Dongping HUANG <dphuang@tencent.com>
// Created: 01/18/12
// Description:

#include "poppy/rpc_handler.h"

#include "thirdparty/gtest/gtest.h"

TEST(RpcHandlerTest, ParseNameAndService)
{
    std::string path = "/rpc/form/rpc_examples.EchoServer.Echo";
    std::string service_full_name;
    std::string method_name;
    ASSERT_TRUE(::poppy::ParseMethodFullNameFromPath(path,
                                                     "/rpc/form",
                                                     &service_full_name,
                                                     &method_name));

    EXPECT_EQ(service_full_name, "rpc_examples.EchoServer");
    EXPECT_EQ(method_name, "Echo");

    path = "/rpc/form/rpc_examples";
    ASSERT_FALSE(::poppy::ParseMethodFullNameFromPath(path,
                                                      "/rpc/form",
                                                      &service_full_name,
                                                      &method_name));
}

