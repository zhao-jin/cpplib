// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: CHEN Feng <phongchen@tencent.com>
// Created: 12/21/11
// Description:

#include "poppy/rpc_controller.h"
#include "thirdparty/gtest/gtest.h"

namespace poppy {

class RpcControllerTest : public testing::Test
{
protected:
    void TestEmpty()
    {
        EXPECT_EQ(0, m_controller.Time());
        EXPECT_EQ(0, m_controller.Timeout());
        EXPECT_EQ(CompressTypeNone, m_controller.RequestCompressType());
        EXPECT_EQ(CompressTypeNone, m_controller.ResponseCompressType());
        EXPECT_EQ("", m_controller.Credential());
        EXPECT_EQ("", m_controller.User());
        EXPECT_EQ("", m_controller.Role());
        EXPECT_EQ(AF_UNSPEC, m_controller.RemoteAddress().Family());
        EXPECT_EQ(RPC_SUCCESS, m_controller.ErrorCode());
        EXPECT_EQ("RPC_SUCCESS", m_controller.ErrorText());
    }

    void TestCompress()
    {
        m_controller.SetRequestCompressType(CompressTypeSnappy);
        EXPECT_EQ(CompressTypeSnappy, m_controller.RequestCompressType());
        m_controller.SetResponseCompressType(CompressTypeSnappy);
        EXPECT_EQ(CompressTypeSnappy, m_controller.ResponseCompressType());
    }

    void TestReset()
    {
        m_controller.SetRequestCompressType(CompressTypeSnappy);
        m_controller.SetResponseCompressType(CompressTypeSnappy);
        m_controller.SetTimeout(9527);
        m_controller.SetFailed("hello, kitty");
        m_controller.Reset();
        TestEmpty();
    }

    void TestFailed()
    {
        m_controller.SetFailed(RPC_ERROR_REQUEST_TIMEOUT);
        EXPECT_EQ(RPC_ERROR_REQUEST_TIMEOUT, m_controller.ErrorCode());

        m_controller.SetFailed("hello, world");
        EXPECT_EQ(RPC_ERROR_FROM_USER, m_controller.ErrorCode());
        EXPECT_EQ("RPC_ERROR_FROM_USER: hello, world", m_controller.ErrorText());
    }

protected:
    RpcController m_controller;
};

TEST_F(RpcControllerTest, DefalutCtor)
{
    TestEmpty();
}

TEST_F(RpcControllerTest, Compress)
{
    TestCompress();
}

TEST_F(RpcControllerTest, Failed)
{
    TestFailed();
}

TEST_F(RpcControllerTest, Cancel)
{
    m_controller.StartCancel();
    EXPECT_TRUE(m_controller.IsCanceled());
}

TEST_F(RpcControllerTest, Credential)
{
    EXPECT_EQ("", m_controller.Credential());
    EXPECT_EQ("", m_controller.User());
    EXPECT_EQ("", m_controller.Role());
}

TEST_F(RpcControllerTest, RemoteAddress)
{
    EXPECT_EQ(AF_UNSPEC, m_controller.RemoteAddress().Family());
}

TEST_F(RpcControllerTest, Time)
{
    EXPECT_EQ(0, m_controller.Time());
}

TEST_F(RpcControllerTest, Timeout)
{
    m_controller.SetTimeout(42);
    EXPECT_EQ(42, m_controller.Timeout());
}

TEST_F(RpcControllerTest, Reset)
{
    TestReset();
}

TEST(RpcController, Dump)
{
    RpcController controller;
    EXPECT_EQ("RpcController: method=, sequence_id=-1, remote_address=, time=0",
              controller.Dump());
    controller.SetFailed("Test Failed");
    EXPECT_EQ("RpcController: method=, sequence_id=-1, remote_address=, "
              "time=0, error=1000(RPC_ERROR_FROM_USER: Test Failed)",
              controller.Dump());
}

} // namespace poppy
