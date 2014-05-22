// Copyright (c) 2010, Tencent Inc.
// All rights reserved.
//
// Author: CHEN Feng <phongchen@tencent.com>

#include <stdlib.h>
#include <iostream>
#include <string>
#include "common/system/net/socket.h"
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

DEFINE_string(command, "stat", "operation command.");
DEFINE_string(server_address, "10.168.151.77:2181", "zookeeper server address.");

bool ProcessCommand(const std::string& cmd, const std::string& server) {
    SocketAddressInet4 server_address(server);
    StreamSocket socket;
    if (!socket.Create()) {
        LOG(ERROR) << "failed to create a stream socket.";
        return false;
    }
    if (!socket.Connect(server_address)) {
        LOG(ERROR) << "failed to connect to server: " << server;
        return false;
    }
    size_t send_size = 0;
    if (!socket.SendAll(cmd.c_str(), cmd.length(), &send_size)) {
        LOG(ERROR) << "failed to send cmd: " << cmd << ", send size: " << send_size;
        return false;
    }
    char buffer[1024];
    size_t received_size = 0;
    std::string s;
    while (socket.Receive(buffer, 1024, &received_size)) {
        std::cout << s.assign(buffer, received_size);
        if (received_size <= 0) {
            break;
        }
    }
    socket.Close();
    return true;
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    std::cout << "#############################################\n";
    std::cout << "[Server: " << FLAGS_server_address
        << "], command: " << FLAGS_command << std::endl;
    ProcessCommand(FLAGS_command, FLAGS_server_address);

    return EXIT_SUCCESS;
}
