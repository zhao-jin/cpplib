// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
//
// An example echo client.

#include <iostream>
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"
#include "poppy/rpc_handler.h"
#include "poppy/test/echo_service.pb.h"
#include "poppy/test/echo_test.h"
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

// using namespace common;

DEFINE_int32(client_thread, 4, "net frame thread");
DEFINE_int32(request_number, 50000, "request number");
DEFINE_string(server_address, "127.0.0.1:50000", "server address");

DEFINE_bool(run_latency_test, false, "run latency test");
DEFINE_bool(run_read_test, false, "run read test");
DEFINE_bool(run_write_test, false, "run write test");
DEFINE_bool(run_read_write_test, false, "run read write test");
DEFINE_bool(run_async_test, false, "run async test");
DEFINE_bool(crash_on_timeout, false, "crash on timeout");

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    rpc_examples::EchoClient client(FLAGS_server_address);

    rpc_examples::EchoClient::s_crash_on_timeout = FLAGS_crash_on_timeout;

    while (true) {
        if (FLAGS_run_latency_test) {
            int throughput = client.RunLatencyTest(FLAGS_request_number);
            std::cout << "Latency test: " << throughput << "/s\n";
        }

        if (FLAGS_run_read_test) {
            int throughput = client.RunReadTest(500);
            std::cout << "Read throughput: " << throughput << "M/s\n";
        }

        if (FLAGS_run_write_test) {
            int throughput = client.RunWriteTest(500);
            std::cout << "Write throughput: " << throughput << "M/s\n";
        }

        if (FLAGS_run_read_write_test) {
            int throughput = client.RunReadWriteTest(500);
            std::cout << "Read/Write throughput: " << throughput << "M/s\n";
        }

        if (FLAGS_run_async_test) {
            client.RunAsyncTest(FLAGS_request_number);
        }
    }

    LOG(INFO) << "All response is back, now quit...";
    return 0;
}

