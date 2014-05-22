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
#include "poppy/examples/echo_form/echo_service.pb.h"
#include "poppy/rpc_controller.h"
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
                      const EchoMessage* request,
                      EchoMessage* response,
                      google::protobuf::Closure* done)
    {
        ++m_request_count;
        response->CopyFrom(*request);
        VLOG(3) << "request: " << request->id();
        VLOG(3) << "response: " << response->id();
        VLOG(3) << "time: " << controller->Time();
        g_total_request_count++;
        done->Run();
    }

    virtual void EchoEmpty(google::protobuf::RpcController* controller,
                           const poppy::EmptyRequest* request,
                           poppy::EmptyResponse* response,
                           google::protobuf::Closure* done)
    {
        ++m_request_count;
        g_total_request_count++;
        done->Run();
    }

#define GENERATE_ECHO_STUB(ns, Type) \
    virtual void Test##Type(::google::protobuf::RpcController* controller, \
                            const ns::Type* request, \
                            ns::Type* response, \
                            ::google::protobuf::Closure* done) \
    { \
        ++m_request_count; \
        response->CopyFrom(*request); \
        VLOG(3) << "Receive " #Type " request"; \
        done->Run(); \
    }

    GENERATE_ECHO_STUB(::poppy::examples::ad, TimeTarget)
    GENERATE_ECHO_STUB(::poppy::examples::ad, AreaTarget)
    GENERATE_ECHO_STUB(::poppy::examples::ad, IPTarget)
    GENERATE_ECHO_STUB(::poppy::examples::ad, WordTarget)
    GENERATE_ECHO_STUB(::poppy::examples::ad, ClassTarget)
    GENERATE_ECHO_STUB(::poppy::examples::ad, BidWord)
    GENERATE_ECHO_STUB(::poppy::examples::ad, Creative)
    GENERATE_ECHO_STUB(::poppy::examples::ad, BrandZone)
    GENERATE_ECHO_STUB(::poppy::examples::ad, BrandCreative)
    GENERATE_ECHO_STUB(::poppy::examples::ad, AdGroup)
    GENERATE_ECHO_STUB(::poppy::examples::ad, AdCampaign)
    GENERATE_ECHO_STUB(::poppy::examples::ad, Advertiser)
    GENERATE_ECHO_STUB(::poppy::examples::ad, PubHeader)
    GENERATE_ECHO_STUB(::poppy::examples::ad, PubRecord)
    GENERATE_ECHO_STUB(::poppy::examples::ad, InvalidValid)
    GENERATE_ECHO_STUB(::poppy::examples::ad, InvalidValidNotify)
    GENERATE_ECHO_STUB(::poppy::examples::ad, PubSlice)
    GENERATE_ECHO_STUB(::poppy::examples::ad, PubSequence)
    GENERATE_ECHO_STUB(::poppy::examples::ad, ModifiedId)
    GENERATE_ECHO_STUB(::poppy::examples::ad, ModifiedIdNotify)
    GENERATE_ECHO_STUB(::poppy::examples::ad, InvalidIdNotifyResponse)

private:
    Atomic<int> m_request_count;
};

} // end namespace rpc_examples

bool g_quit = false;

static void SignalIntHandler(int sig_no)
{
    g_quit = true;
}

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

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
        // fprintf(stderr, "request rate=%d, total request=%d\n",
        //        echo_service->GetAndClearRequestCount(), g_total_request_count);
    }

    return EXIT_SUCCESS;
}
