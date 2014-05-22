// Copyright (c) 2012, Tencent Inc.
// All rights reserved.
//
// Author: Dongping HUANG <dphuang@tencent.com>
// Created: 01/17/12
// Description:

#include <iostream>

#include "common/base/singleton.h"
#include "common/net/http/http_downloader.h"
#include "common/net/http/http_server.h"
#include "poppy/test/echo_server.h"
#include "poppy/test/echo_test.h"
#include "thirdparty/gtest/gtest.h"
#include "thirdparty/perftools/heap-checker.h"

using namespace common;

class RpcServerPagesTest : public testing::Test
{
protected:
    virtual void SetUp() {
        netframe::NetFrame *netframe = new netframe::NetFrame();
        m_test_server = new rpc_examples::TestServer(netframe);
        SocketAddressInet4 real_bind_address;
        m_test_server->Start("127.0.0.1:0", &real_bind_address);
        m_server_address = real_bind_address.ToString();
    }
    virtual void TearDown() {
        delete m_test_server;
    }

protected:
    std::string m_server_address;

private:
    rpc_examples::TestServer* m_test_server;
};

TEST_F(RpcServerPagesTest, StatusPage)
{
    HttpDownloader downloader;
    HttpResponse response;
    HttpDownloader::ErrorType error;
    EXPECT_TRUE(downloader.Get("http://" + m_server_address + "/status", &response, &error));
    EXPECT_EQ(HttpDownloader::SUCCESS, error);
    EXPECT_EQ(HttpResponse::Status_OK, response.status());
    ::std::cout << response.HeadersToString() << ::std::endl;
    // ::std::cout << response.http_body() << ::std::endl;
}

TEST_F(RpcServerPagesTest, VarsPage)
{
    HttpDownloader downloader;
    HttpResponse response;
    HttpDownloader::ErrorType error;
    EXPECT_TRUE(downloader.Get("http://" + m_server_address + "/vars", &response, &error));
    EXPECT_EQ(HttpDownloader::SUCCESS, error);
    EXPECT_EQ(HttpResponse::Status_OK, response.status());
    ::std::cout << response.HeadersToString() << ::std::endl;
    // ::std::cout << response.http_body() << ::std::endl;
}

TEST_F(RpcServerPagesTest, VersionPage)
{
    HttpDownloader downloader;
    HttpResponse response;
    HttpDownloader::ErrorType error;
    EXPECT_TRUE(downloader.Get("http://" + m_server_address + "/version", &response, &error));
    EXPECT_EQ(HttpDownloader::SUCCESS, error);
    EXPECT_EQ(HttpResponse::Status_OK, response.status());
    ::std::cout << response.HeadersToString() << ::std::endl;
    // ::std::cout << response.http_body() << ::std::endl;
}

TEST_F(RpcServerPagesTest, HealthPage)
{
    HttpDownloader downloader;
    HttpResponse response;
    HttpDownloader::ErrorType error;
    EXPECT_TRUE(downloader.Get("http://" + m_server_address + "/health", &response, &error));
    EXPECT_EQ(HttpDownloader::SUCCESS, error);
    EXPECT_EQ(HttpResponse::Status_OK, response.status());
    ::std::cout << response.HeadersToString() << ::std::endl;
    // ::std::cout << response.http_body() << ::std::endl;
}

TEST_F(RpcServerPagesTest, FlagsPage)
{
    HttpDownloader downloader;
    HttpResponse response;
    HttpDownloader::ErrorType error;
    EXPECT_TRUE(downloader.Get("http://" + m_server_address + "/flags", &response, &error));
    EXPECT_EQ(HttpDownloader::SUCCESS, error);
    EXPECT_EQ(HttpResponse::Status_OK, response.status());
    ::std::cout << response.HeadersToString() << ::std::endl;
    // ::std::cout << response.http_body() << ::std::endl;
}

TEST_F(RpcServerPagesTest, ProfilePage)
{
    HttpDownloader downloader;
    HttpResponse response;
    HttpDownloader::ErrorType error;
    std::string url = "http://" + m_server_address + "/pprof/profile?seconds=1";
    EXPECT_TRUE(downloader.Get(url, &response, &error));
    EXPECT_EQ(HttpDownloader::SUCCESS, error);
    EXPECT_EQ(HttpResponse::Status_OK, response.status());
    ::std::cout << response.HeadersToString() << ::std::endl;
    // ::std::cout << response.http_body() << ::std::endl;
}
