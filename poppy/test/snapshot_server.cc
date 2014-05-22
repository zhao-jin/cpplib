// Copyright 2011, Tencent Inc.
// Author: Eric Liu <ericliu@tencent.com>
//
// An example snapshot server.

// For graceful exit.
#include <signal.h>

// For exported variables.
#include "common/base/export_variable.h"

// Headers which must be included for rpc servers.
#include "poppy/rpc_server.h"
#include "poppy/test/snapshot_service.pb.h"

// For request counting.
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/thread.h"

// includes from thirdparty
#include "gflags/gflags.h"
#include "glog/logging.h"

DEFINE_string(server_address, "127.0.0.1:50000", "the address server listen on.");

// Test exported variables.
int g_total_request_count;
EXPORT_VARIABLE(total_request_count, &g_total_request_count);

// using namespace common;
namespace rpc_examples {

class SnapshotServerImpl : public SnapshotServer
{
public:
    SnapshotServerImpl()
        : m_add_request_count(0),
          m_query_request_count(0) {
    }
    virtual ~SnapshotServerImpl() {}

    int GetAndClearAddRequestCount() {
        return m_add_request_count.Exchange(0);
    }

    int GetAndClearQueryRequestCount() {
        return m_query_request_count.Exchange(0);
    }

private:
    virtual void AddSnapshot(google::protobuf::RpcController* controller,
                             const AddSnapshotRequest* request,
                             poppy::EmptyResponse* response,
                             google::protobuf::Closure* done) {
        ++m_add_request_count;
        m_docid = request->docid();
        m_content = request->content();
        g_total_request_count++;
        done->Run();
        LOG(INFO) << "add snapshot request: docid=" << m_docid
                  << " content_len=" << m_content.length();
    }

    virtual void QuerySnapshot(google::protobuf::RpcController* controller,
                               const QuerySnapshotRequest* request,
                               QuerySnapshotResponse* response,
                               google::protobuf::Closure* done) {
        ++m_query_request_count;
        response->set_content(m_content);
        g_total_request_count++;
        done->Run();
    }

    Atomic<int> m_add_request_count;
    Atomic<int> m_query_request_count;

    uint64_t m_docid;
    std::string m_content;
};

} // end namespace rpc_examples

bool g_quit = false;

static void SignalIntHandler(int sig)
{
    g_quit = true;
}

int main(int argc, char** argv)
{
    FLAGS_v = 3;
    FLAGS_log_dir = ".";
    google::ParseCommandLineFlags(&argc, &argv, true);

    google::InitGoogleLogging(argv[0]);
    // Define an rpc server.
    poppy::RpcServer rpc_server;
    rpc_examples::SnapshotServerImpl* snapshot_service = new rpc_examples::SnapshotServerImpl();
    // Register an rpc service.
    rpc_server.RegisterService(snapshot_service);

    // Start rpc server.
    if (!rpc_server.Start(FLAGS_server_address)) {
        LOG(WARNING) << "Failed to start server.";
        return EXIT_FAILURE;
    }

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);

    while (!g_quit) {
        ThisThread::Sleep(1000);
        LOG(INFO) << "add request rate=" << snapshot_service->GetAndClearAddRequestCount()
                  << ", query request rate=" << snapshot_service->GetAndClearQueryRequestCount()
                  << ", total request=" << g_total_request_count;
    }

    return EXIT_SUCCESS;
}
