// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
// Eric Liu <ericliu@tencent.com>
//
// An example echo server.

// For graceful exit.
#include <signal.h>

// For exported variables.
#include "common/base/export_variable.h"

// Headers which must be included for rpc servers.
#include "poppy/examples/echo_timeout/echo_service.pb.h"
#include "poppy/rpc_server.h"

// For request counting.
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/thread.h"

// includes from thirdparty
#include "gflags/gflags.h"
#include "glog/logging.h"

// using namespace common;

DEFINE_string(server_address, "127.0.0.1:10000", "the address server listen on.");

// Test exported variables.
int g_total_request_count;
EXPORT_VARIABLE(total_request_count, &g_total_request_count);

namespace rpc_examples {

class EchoServerImpl : public EchoServer
{
public:
    EchoServerImpl()
        : m_request_count(0) {
    }
    virtual ~EchoServerImpl() {}

    int GetAndClearRequestCount() {
        return m_request_count.Exchange(0);
    }

private:
    virtual void Echo(google::protobuf::RpcController* controller,
                      const EchoRequest* request,
                      EchoResponse* response,
                      google::protobuf::Closure* done) {
        ++m_request_count;
        // ģ�ⳬʱ
        ThisThread::Sleep(2000);
        response->set_user(request->user());
        response->set_message(
            "echo from server: " + FLAGS_server_address +
            ", message: " + request->message());
        LOG(INFO) << "request: " << request->message();
        LOG(INFO) << "response: " << response->message();
        g_total_request_count++;
        done->Run();
    }

    Atomic<int> m_request_count;
};

} // end namespace rpc_examples

bool g_quit = false;

static void SignalIntHandler(int sig)
{
    g_quit = true;
}

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    // Define an rpc server.
    poppy::RpcServer rpc_server;
    rpc_examples::EchoServerImpl* echo_service = new rpc_examples::EchoServerImpl();
    // Register an rpc service.
    rpc_server.RegisterService(echo_service);

    // Start rpc server.
    if (!rpc_server.Start(FLAGS_server_address)) {
        LOG(WARNING) << "Failed to start server.";
        return EXIT_FAILURE;
    }

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);

    while (!g_quit) {
        ThisThread::Sleep(1000);
        fprintf(stderr, "request rate=%d, total request=%d\n",
                echo_service->GetAndClearRequestCount(), g_total_request_count);
    }

    return EXIT_SUCCESS;
}
