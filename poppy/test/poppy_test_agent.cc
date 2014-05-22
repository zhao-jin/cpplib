// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Simon Wang <simonwang@tencent.com>
// Created: 12/20/11
// Description: Server Agent to Start/Stop/EnableHealthy/DisableHealthy

#include <signal.h>

#include <sstream>
#include <string>

#include "common/base/string/concat.h"
#include "common/netframe/netframe.h"
#include "common/netframe/socket_handler.h"
#include "common/system/concurrency/this_thread.h"
#include "common/system/io/directory.h"
#include "common/system/process.h"
#include "poppy/test/poppy_test_command.h"

#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

using namespace netframe;

DEFINE_string(agent_listen_ip, "127.0.0.1", "listen address");
DEFINE_int32(agent_listen_port, 50004, "listen port");
DEFINE_int32(echo_server_1_port, 50000, "echo server 1 port");
DEFINE_int32(echo_server_2_port, 50001, "echo server 2 port");
DEFINE_string(server_name, "echoserver", "server executable file name");

bool g_quit = false;

static void SignalIntHandler(int sig)
{
    g_quit = true;
}

class PoppyServerAgent
{
public:
    bool StartServer(int index, bool credential = false)
    {
        CheckServerIndex(index);

        if (m_server_process[index].IsValid()) {
            LOG(ERROR) << "server " << index << " already started.";
            return false;
        }

        std::string server_address = GetServerAddress(index);
        std::string server_name = GetServerName(index);
        std::string current_dir = io::directory::GetCurrentDir();
        std::string file_path;
        std::string arguments;
        file_path = StringConcat(current_dir, "/", server_name);
        arguments = "--server_address=";
        arguments += server_address;
        if (credential) {
            LOG(ERROR) << " not supported now";
        }
        m_server_process[index] = Process::Start(file_path, arguments);
        return true;
    }

    void StopAllServers()
    {
        for (int i = 0; i < kServerNum; ++i) {
            if (m_server_process[i].IsValid())
                StopServer(i);
        }
    }

    bool StopServer(int index)
    {
        CheckServerIndex(index);

        if (!m_server_process[index].IsValid()) {
            LOG(ERROR) << "server " << index << " already stopped.";
            return false;
        }

        return m_server_process[index].Terminate();
    }

    bool EnableServer(int index) {
        CheckServerIndex(index);

        if (!m_server_process[index].IsValid()) {
            LOG(ERROR) << "server " << index << " already stopped.";
            return false;
        }

        m_server_process[index].SendSignal(SIGUSR1);
        return true;
    }

    bool DisableServer(int index) {
        CheckServerIndex(index);

        if (!m_server_process[index].IsValid()) {
            LOG(ERROR) << "server " << index << " already stopped.";
            return false;
        }

        m_server_process[index].SendSignal(SIGUSR2);
        return true;
    }

private:
    void CheckServerIndex(int index)
    {
        CHECK(index >= 0 && index < kServerNum)
            << " server index out of range,"
            << "make sure it's greater than 0 and less than "
            << kServerNum;
    }

    std::string GetServerAddress(int index)
    {
        CheckServerIndex(index);

        std::string ip = FLAGS_agent_listen_ip;
        int port = 0;
        if (index == 0) {
            port = FLAGS_echo_server_1_port;
        } else if (index == 1) {
            port = FLAGS_echo_server_2_port;
        } else {
            CHECK(false) << " index must be 0 or 1";
        }
        return StringConcat(ip, ":", port);
    }

    std::string GetServerName(int index)
    {
        CheckServerIndex(index);

        return StringConcat(FLAGS_server_name, "_", index);
    }

private:
    static const int kServerNum = 2;
    Process m_server_process[kServerNum];
};

class AgentStreamSocketHandler : public LineStreamSocketHandler
{
public:
    AgentStreamSocketHandler(NetFrame* netframe, PoppyServerAgent* agent)
        : LineStreamSocketHandler(*netframe), m_agent(agent)
    {
    }

    ~AgentStreamSocketHandler()
    {
    }

    virtual bool OnSent(Packet* packet)
    {
        LOG(INFO) << "OnSent";
        return true;
    }

    void OnReceived(const Packet& packet)
    {
        const void* content = packet.Content();
        size_t length = packet.Length();
        std::string msg;
        msg.assign(static_cast<char*>(const_cast<void*>(content)), length);
        std::string command;
        int index;
        std::stringstream ss(msg);
        ss >> command >> index;
        LOG(INFO) << "receive command: " << command << " index: " << index;
        bool result = false;
        if (command == poppy::test::kStartServerCommand) {
            result = m_agent->StartServer(index);
        } else if (command == poppy::test::kStartCredentialCommand) {
            result = m_agent->StartServer(index, true);
        } else if (command == poppy::test::kStopServerCommand) {
            result = m_agent->StopServer(index);
        } else if (command == poppy::test::kEnableServerCommand) {
            result = m_agent->EnableServer(index);
        } else if (command == poppy::test::kDisableServerCommand) {
            result = m_agent->DisableServer(index);
        } else {
            LOG(ERROR) << "receive unknown command: " << command;
        }
        SendResponse(result);
    }

    virtual void OnConnected() {}

    virtual void OnClose(int error_code)
    {
        LOG(INFO) << "OnClosed: " << strerror(error_code);
    }

private:
    void SendResponse(bool succ)
    {
        std::string response;
        if (succ) {
            response = "succ\n";
        } else {
            response = "fail\n";
        }
        if (GetNetFrame().SendPacket(GetEndPoint(), response.c_str(),
                                     response.size() ) < 0) {
            LOG(ERROR) << "send response error";
        }
        LOG(INFO) << "send response " << response;
    }

private:
    PoppyServerAgent* m_agent;
};

class AgentListenSocketHandler : public ListenSocketHandler
{
public:
    AgentListenSocketHandler(NetFrame* netframe, PoppyServerAgent* agent)
        : ListenSocketHandler(*netframe), m_agent(agent)
    {
    }

    StreamSocketHandler* OnAccepted(SocketId id)
    {
        return new AgentStreamSocketHandler(&GetNetFrame(), m_agent);
    }

    void OnClose(int error_code)
    {
        LOG(INFO) << "OnClosed";
    }

private:
    PoppyServerAgent* m_agent;
};

int main(int argc, char** argv)
{
    FLAGS_log_dir = '.';
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);

    PoppyServerAgent agent;
    NetFrame netframe;

    std::string server_address = StringConcat(FLAGS_agent_listen_ip,
                                              ":",
                                              FLAGS_agent_listen_port);
    if (netframe.AsyncListen(SocketAddressInet(server_address),
                             new AgentListenSocketHandler(&netframe, &agent),
                             32768) < 0) {
        LOG(ERROR) << "listen on " << server_address << " error";
        return EXIT_FAILURE;
    }

    while (!g_quit) {
        ThisThread::Sleep(1000);
    }

    agent.StopAllServers();

    return EXIT_SUCCESS;
}
