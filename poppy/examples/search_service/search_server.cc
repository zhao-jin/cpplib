// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>
//
// A search application sample.

#include <signal.h>
#include "common/base/export_variable.h"
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/thread.h"
#include "common/system/concurrency/thread_pool.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "poppy/examples/search_service/search_service.pb.h"
#include "poppy/rpc_server.h"

// using namespace common;

DEFINE_string(server_address, "127.0.0.1:10000", "the address server listen on.");

int g_total_request_count;
EXPORT_VARIABLE(total_request_count, &g_total_request_count);

class SearchServerImpl : public SearchServer
{
public:
    SearchServerImpl() {}
    virtual ~SearchServerImpl() {}

private:
    virtual void Search(google::protobuf::RpcController* controller,
                        const SearchRequest* request,
                        SearchResponse* response,
                        google::protobuf::Closure* done) {
        // std::string query = request->query();
        // search query results...
        // if we get docid 10 scored 10.1 and docid 2 scored 8.5

        SearchResponse::Result* item = response->add_result();
        item->set_doc_id(10);
        item->set_score(10.1);

        item = response->add_result();
        item->set_doc_id(2);
        item->set_score(8.5);

        ++g_total_request_count;
        done->Run();
    }

    ThreadPool m_thread_pool;

    // Maybe this function will take long time to complete.
    // We can run it in another thread.
    virtual void DoDebugSearch(google::protobuf::RpcController* controller,
                               const SearchRequest* request,
                               SearchResponse* response,
                               google::protobuf::Closure* done) {
        Closure<void>* callback = NewClosure(this,
                                             &SearchServerImpl::DoTrueDebugSearch,
                                             controller,
                                             request,
                                             response,
                                             done);
        m_thread_pool.AddTask(callback);
    }

    void DoTrueDebugSearch(google::protobuf::RpcController* controller,
                           const SearchRequest* request,
                           SearchResponse* response,
                           google::protobuf::Closure* done) {
        // If we get only one result:
        SearchResponse::Result* item = response->add_result();
        item->set_doc_id(10);
        item->set_score(10.1);

        ++g_total_request_count;
        done->Run();
    }
};

bool g_quit = false;

static void SignalIntHandler(int sig)
{
    g_quit = true;
}

#include "common/net/http/http_handler.h"

void HandleHttpRequestSample(const HttpRequest* request,
                             HttpResponse* response,
                             Closure<void>* done)
{
    response->SetHeader("Content-Type", "text/plain");
    std::string* http_body = response->mutable_http_body();
    http_body->assign("This is a test!");
    done->Run();
}

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    // Define an rpc server.
    poppy::RpcServer rpc_server;

    HttpClosure* closure = NewPermanentClosure(HandleHttpRequestSample);
    SimpleHttpServerHandler* handler = new SimpleHttpServerHandler(closure);
    rpc_server.RegisterHandler("/my_procedure_path", handler);

    SearchServerImpl* search_service = new SearchServerImpl();
    // Register an rpc service.
    rpc_server.RegisterService(search_service);

    // Start rpc server.
    if (!rpc_server.Start(FLAGS_server_address)) {
        LOG(WARNING) << "Failed to start server.";
        return EXIT_FAILURE;
    }

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);

    while (!g_quit) {
        ThisThread::Sleep(1000);
    }

    return EXIT_SUCCESS;
}

