// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Simon Wang <simonwang@tencent.com>
// Created: 12/22/11
// Description: Server Controller for Client Test Case

#ifndef POPPY_TEST_SERVER_CONTROLLER_H
#define POPPY_TEST_SERVER_CONTROLLER_H
#pragma once

#include <string>

#include "common/netframe/netframe.h"
#include "common/netframe/socket_handler.h"
#include "common/system/net/socket.h"

#include "poppy/test/echo_server.h"

namespace poppy {
namespace test  {

static const int kServerNum = 2;

struct LaunchOptions
{
    LaunchOptions() : use_agent(false),
                      agent_listen_ip(""),
                      agent_listen_port(0)
    {
        for (int i = 0; i < kServerNum; ++i) {
            server_listen_port[i] = 0;
        }
    }

    LaunchOptions(const LaunchOptions& options) : use_agent(options.use_agent),
        agent_listen_ip(options.agent_listen_ip),
        agent_listen_port(options.agent_listen_port)
    {
        for (int i = 0; i < kServerNum; ++i) {
            server_listen_port[i] = options.server_listen_port[i];
        }
    }

    bool use_agent;
    std::string agent_listen_ip;
    int agent_listen_port;
    int server_listen_port[kServerNum];
};

class ServerController
{
public:
    explicit ServerController(LaunchOptions options);

    ~ServerController();

    bool StartServer(int index, bool use_credential = false,
                     common::CredentialVerifier* credential_verifier = NULL);

    bool StopServer(int index);

    bool EnableServer(int index);

    bool DisableServer(int index);

    std::string GetServerAddress(int index);

    std::string GetInvalidAddress();

private:
    bool StartLocalServer(int index, bool use_credential,
                          common::CredentialVerifier* credential_verifier);
    bool StartRemoteServer(int index, bool use_credential);

    bool StopLocalServer(int index);
    bool StopRemoteServer(int index);

    bool EnableLocalServer(int index);
    bool EnableRemoteServer(int index);

    bool DisableLocalServer(int index);
    bool DisableRemoteServer(int index);

    bool CheckResponse();
    void SendCommand(const std::string& command, int index);

    void CheckServerIndex(int index);

    SocketAddressInet4 m_agent_address;
    StreamSocket m_agent_connector;

    LaunchOptions m_launch_options;

    rpc_examples::TestServer* m_test_server[kServerNum];
};

} // namespace test
} // namespace poppy

#endif // POPPY_TEST_SERVER_CONTROLLER_H
