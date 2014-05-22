// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Simon Wang <simonwang@tencent.com>
// Created: 12/22/11
// Description: Server Controller Class

#include "poppy/test/server_controller.h"

#include "common/base/string/concat.h"
#include "poppy/test/poppy_test_command.h"

namespace poppy {
namespace test {

const int kMaxReceiveBufferSize = 1024;

ServerController::ServerController(LaunchOptions options)
                    : m_launch_options(options)
{
    if (options.use_agent) {
        CHECK(!options.agent_listen_ip.empty())
            << " agent address can't be empty";
        CHECK(options.agent_listen_port > 0
              && options.agent_listen_port < 65536)
            << " agent listen port out of range";
        for (int i = 0; i < kServerNum; ++i) {
            CHECK(options.agent_listen_port
                  != options.server_listen_port[i])
                << " agent listen port can't be equal to test server listen port";
            CHECK(options.server_listen_port[i] >= 0
                  && options.server_listen_port[i] < 65536)
                << " test server listen port out of range";
        }
        std::string agent_address = StringConcat(options.agent_listen_ip, ":",
                                                 options.agent_listen_port);
        m_agent_address.Assign(agent_address);
        m_agent_connector.Create(AF_INET, IPPROTO_TCP);
        m_agent_connector.Connect(m_agent_address);
    } else {
        for (int i = 0; i < kServerNum; ++i) {
            CHECK(options.server_listen_port[i] >= 0
                  && options.server_listen_port[i] < 65536)
                << " test server listen port out of range";
            m_test_server[i] = new rpc_examples::TestServer();
        }
    }
}

ServerController::~ServerController()
{
    if (m_launch_options.use_agent) {
        m_agent_connector.Shutdown();
    } else {
        for (int i = 0; i < kServerNum; ++i) {
            StopServer(i);
        }
    }
}

bool ServerController::StartServer(int index, bool use_credential,
                                   common::CredentialVerifier* credential_verifier)
{
    CheckServerIndex(index);

    if (m_launch_options.use_agent) {
        return StartRemoteServer(index, use_credential);
    } else {
        return StartLocalServer(index, use_credential, credential_verifier);
    }
}

bool ServerController::StopServer(int index)
{
    CheckServerIndex(index);

    if (m_launch_options.use_agent) {
        return StopRemoteServer(index);
    } else {
        return StopLocalServer(index);
    }
}

bool ServerController::EnableServer(int index)
{
    CheckServerIndex(index);

    if (m_launch_options.use_agent) {
        return EnableRemoteServer(index);
    } else {
        return EnableLocalServer(index);
    }
}

bool ServerController::DisableServer(int index)
{
    CheckServerIndex(index);

    if (m_launch_options.use_agent) {
        return DisableRemoteServer(index);
    } else {
        return DisableLocalServer(index);
    }
}

void ServerController::CheckServerIndex(int index)
{
    CHECK(index >= 0 && index < kServerNum)
        << " server index out of range,"
        << "make sure it's greater than 0 and less than "
        << kServerNum;
}

////////////////////////////////////////////////////////
bool ServerController::StartRemoteServer(int index, bool use_credential)
{
    if (use_credential) {
        SendCommand(poppy::test::kStartCredentialCommand, index);
    } else {
        SendCommand(poppy::test::kStartServerCommand, index);
    }
    return CheckResponse();
}

bool ServerController::StartLocalServer(int index, bool use_credential,
                                        common::CredentialVerifier* credential_verifier)
{
    if (!m_test_server[index]) {
        if (use_credential) {
            m_test_server[index] = new rpc_examples::TestServer(0, credential_verifier);
        } else {
            m_test_server[index] = new rpc_examples::TestServer(0);
        }
    }
    SocketAddressInet4 real_bind_address;
    m_test_server[index]->Start(GetServerAddress(index), &real_bind_address);
    m_launch_options.server_listen_port[index] = real_bind_address.GetPort();
    return true;
}

bool ServerController::StopRemoteServer(int index)
{
    SendCommand(poppy::test::kStopServerCommand, index);
    return CheckResponse();
}

bool ServerController::StopLocalServer(int index)
{
    delete m_test_server[index];
    m_test_server[index] = NULL;
    return true;
}

bool ServerController::EnableRemoteServer(int index)
{
    SendCommand(poppy::test::kEnableServerCommand, index);
    return CheckResponse();
}

bool ServerController::EnableLocalServer(int index)
{
    if (!m_test_server[index]) {
        return false;
    }

    m_test_server[index]->SetHealthy(true);
    return true;
}

void ServerController::SendCommand(const std::string& command, int index)
{
    std::string msg = StringConcat(command, " ", index, "\n");
    m_agent_connector.SendAll(msg.data(), msg.size());
}

bool ServerController::DisableRemoteServer(int index)
{
    SendCommand(poppy::test::kDisableServerCommand, index);
    return CheckResponse();
}

bool ServerController::DisableLocalServer(int index)
{
    if (!m_test_server[index]) {
        return false;
    }

    m_test_server[index]->SetHealthy(false);
    return true;
}

bool ServerController::CheckResponse()
{
    char buffer[kMaxReceiveBufferSize];
    size_t received_length;
    bool ret = m_agent_connector.ReceiveLine(buffer,
                                             kMaxReceiveBufferSize,
                                             &received_length);
    std::string resp(buffer, received_length);
    if (!ret || resp != "succ\n") {
        return false;
    }
    return true;
}

std::string ServerController::GetServerAddress(int index) {
    CheckServerIndex(index);

    std::string ip;
    if (m_launch_options.use_agent) {
        ip = m_launch_options.agent_listen_ip;
    } else {
        ip = "127.0.0.1";
    }
    int port = m_launch_options.server_listen_port[index];
    return StringConcat(ip, ":", port);
}

std::string ServerController::GetInvalidAddress()
{
    std::string ip;
    int port = 10000;
    for (int i = 10000; i < 65536; ++i) {
        bool collapse = false;
        for (int j = 0; j < kServerNum; ++j) {
            if (i == m_launch_options.server_listen_port[j]) {
                collapse = true;
                break;
            }
        }
        if (!collapse) {
            port = i;
            break;
        }
    }
    if (m_launch_options.use_agent) {
        ip = m_launch_options.agent_listen_ip;
    } else {
        ip = "127.0.0.1";
    }
    return StringConcat(ip, ":", port);
}

} // namespace test
} // namespace poppy
