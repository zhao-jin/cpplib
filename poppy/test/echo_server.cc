// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
//
// An example echo server.

#include <signal.h>

#include <iostream>
#include <string>

#include "common/base/export_variable.h"
#include "common/base/scoped_ptr.h"
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/thread.h"
#include "common/system/concurrency/thread_pool.h"
#include "poppy/rpc_controller.h"
#include "poppy/rpc_handler.h"
#include "poppy/rpc_server.h"
#include "poppy/test/echo_server.h"
#include "poppy/test/echo_service.pb.h"
// includes from thirdparty
#include "gflags/gflags.h"
#include "glog/logging.h"

// using namespace common;

DEFINE_int32(server_thread, 4, "server netframe thread number");
DEFINE_string(server_address, "127.0.0.1:50000", "the server address");
DEFINE_string(log_name_prefix, "", "log file name prefix");

float g_test_float_number = 3.14159265f;
EXPORT_VARIABLE(float_number, &g_test_float_number);

bool g_quit = false;
bool g_enable_healthy = true;

static void SignalIntHandler(int sig)
{
    g_quit = true;
}

static void SignalUsrHandler(int signo)
{
    if (signo == SIGUSR1) {
        g_enable_healthy = true;
    } else if (signo == SIGUSR2) {
        g_enable_healthy = false;
    }
}

int main(int argc, char** argv)
{
    FLAGS_v = 3;
    google::ParseCommandLineFlags(&argc, &argv, true);
    std::string log_dir = FLAGS_log_dir;
    if (!FLAGS_log_name_prefix.empty()) {
        log_dir += "/";
        log_dir += FLAGS_log_name_prefix;
        google::SetLogDestination(google::INFO, log_dir.c_str());
    }
    google::InitGoogleLogging(argv[0]);

    rpc_examples::TestServer server(FLAGS_server_thread);

    if (!server.Start(FLAGS_server_address)) {
        return EXIT_FAILURE;
    }

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);
    signal(SIGUSR1, SignalUsrHandler);
    signal(SIGUSR2, SignalUsrHandler);

    while (!g_quit) {
        ThisThread::Sleep(1000);
        VLOG(3) << "request rate=" << server.GetAndClearRequestCount();
        server.SetHealthy(g_enable_healthy);
    }

    return EXIT_SUCCESS;
}
