// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>
//
// A search application sample.

#include "common/base/string/algorithm.h"
#include "common/system/concurrency/this_thread.h"
#include "poppy/examples/search_service/search_service.pb.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"
// includes from thirdparty
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

// using namespace common;

bool g_complete = false;
DEFINE_string(server_address, "127.0.0.1:10000", "the address server listen on.");

void CallbackFunction(
    poppy::RpcController* controller,
    SearchRequest* request,
    SearchResponse* response)
{
    if (controller->Failed()) {
        LOG(INFO) << "failed: " << controller->ErrorText();
    } else {
        for (int i = 0; i < response->result_size(); i++) {
            const SearchResponse::Result& result = response->result(i);
            LOG(INFO) << "docid: " << result.doc_id() << "\t" << "score: " << result.score();
        }
    }

    delete controller;
    delete request;
    delete response;
    g_complete = true;
}

void AsyncCall()
{
    poppy::RpcClient rpc_client;
    poppy::RpcChannel rpc_channel(&rpc_client, FLAGS_server_address);
    SearchServer::Stub search_client(&rpc_channel);

    poppy::RpcController* controller = new poppy::RpcController;
    SearchRequest* request = new SearchRequest;
    SearchResponse* response = new SearchResponse;

    request->set_query("test");
    Closure<void>* done = NewClosure(CallbackFunction, controller, request, response);
    search_client.DoDebugSearch(controller, request, response, done);

    // Wait for the response before the client destroys.
    while (!g_complete) {
        ThisThread::Sleep(1000);
    }
}

void SyncCall()
{
    // Define an rpc client. It's shared by all rpc channels.
    poppy::RpcClient rpc_client;
    // Define an rpc channel. It connects to a specified group of servers.
    poppy::RpcChannel rpc_channel(&rpc_client, FLAGS_server_address);
    // Define an rpc stub. It use a specifed channel to send requests.
    SearchServer::Stub search_client(&rpc_channel);

    poppy::RpcController rpc_controller;
    SearchRequest request;
    SearchResponse response;

    request.set_query("test");
    search_client.Search(&rpc_controller, &request, &response, NULL);

    if (rpc_controller.Failed()) {
        LOG(INFO) << "failed: " << rpc_controller.ErrorText();
    } else {
        for (int i = 0; i < response.result_size(); i++) {
            const SearchResponse::Result& result = response.result(i);
            LOG(INFO) << "docid: " << result.doc_id() << "\t"
                      << "score: " << result.score();
        }
    }
}

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    LOG(INFO) << "A sync call ...";
    SyncCall();
    LOG(INFO) << "A async call ...";
    AsyncCall();

    return EXIT_SUCCESS;
}
