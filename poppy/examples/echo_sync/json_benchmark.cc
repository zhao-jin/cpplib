// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Eric Liu <ericliu@tencent.com>
// Created: 08/04/11
// Description:

#include <stdlib.h>
#include "common/system/net/socket.h"
#include "common/system/time/timestamp.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

// using namespace common;

DEFINE_string(server_address, "127.0.0.1:10000", "server address");
DEFINE_int32(total_request_count, 10000, "send total request count");

int main(int argc, char* argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    StreamSocket socket;

    if (!socket.Create()) {
        LOG(ERROR) << "Failed create sockect.";
        return EXIT_FAILURE;
    }

    SocketAddressInet4 server_address(FLAGS_server_address);

    if (!socket.Connect(server_address)) {
        LOG(ERROR) << "Failed connect to server " << FLAGS_server_address << ".";
        return EXIT_FAILURE;
    }

    char kRequestData[] =
        "POST /rpc/rpc_examples.EchoServer.EchoMessage HTTP/1.1\r\n"
        "Host: 127.0.0.1:10000\r\n"
        "Accept-Encoding: identity\r\n"
        "Content-Length: 49\r\n"
        "Connection: Keep-Alive\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "\r\n"
        "{\"message\": \"hello,中国人\", \"user\": \"ericliu\"}";

    char kResponseData[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: 91\r\n"
        "\r\n"
        "{\"message\":\"echo from server: 127.0.0.1:10000, message: hello,中国人\","
        "\"user\":\"ericliu\"}\n";

    int total_count = 0;
    int success_count = 0;
    uint64_t t = GetTimeStampInUs();

    while (total_count < FLAGS_total_request_count) {
        total_count++;

        if (!socket.SendAll(kRequestData, strlen(kRequestData))) {
            LOG(ERROR) << "Failed send request.";
            return EXIT_FAILURE;
        }

        char response[1024] = {0};
        size_t received_size = 0;
        if (!socket.Receive(response, sizeof(response), &received_size)) {
            LOG(ERROR) << "Failed receive response." << response;
            return EXIT_FAILURE;
        }

        if (strcmp(response, kResponseData))
            LOG(ERROR) << response;

        success_count++;
    }

    t = GetTimeStampInUs() - t;
    uint64_t throughput = total_count * 1000000.0 / t;
    LOG(INFO) << "Json C++ Client Speed = " << throughput << " /s";

    socket.Close();
    return EXIT_SUCCESS;
}
