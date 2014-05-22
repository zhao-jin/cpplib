// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Eric Liu <ericliu@tencent.com>
// Created: 07/11/11
// Description:

#include <signal.h>
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/this_thread.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"
#include "poppy/test/snapshot_service.pb.h"

// read data file
#include "common/system/io/textfile.h"

// includes from thirdparty
#include "gflags/gflags.h"
#include "glog/logging.h"

// using namespace common;

DEFINE_string(server_address, "127.0.0.1:50000", "server address");
DEFINE_int32(request_number, 300, "request number");
DEFINE_string(data_file, "qq.dat", "the snapshot data file path.");

uint64_t docid = 12298956196516809855ULL;

Atomic<int> g_request_count = 0;

void SendRequest(rpc_examples::SnapshotServer::Stub* snapshot_client);

bool g_quit = false;

void SignalIntHandler(int sig)
{
    g_quit = true;
}

namespace rpc_examples {

class ConnectionManager
{
public:
    static ConnectionManager* instance() {
        return &s_instance;
    }
    void SetServerAddress(const std::string& servers) {
        m_server_addresses = servers;
    }
    poppy::RpcChannel* GetChannel() {
        if (!m_rpc_channel) {
            m_rpc_channel =
                new poppy::RpcChannel(&m_rpc_client, m_server_addresses);
        }

        return m_rpc_channel;
    }
private:
    ConnectionManager() : m_rpc_channel(0) {}
    ~ConnectionManager() {
        delete m_rpc_channel;
    }

private:
    static ConnectionManager s_instance;
    std::string m_server_addresses;
    poppy::RpcClient m_rpc_client;
    poppy::RpcChannel* m_rpc_channel;
};

} // namespace rpc_examples

rpc_examples::ConnectionManager rpc_examples::ConnectionManager::s_instance;

void QueryCallback(rpc_examples::SnapshotServer::Stub* snapshot_client,
                   poppy::RpcController* controller,
                   rpc_examples::QuerySnapshotRequest* request,
                   rpc_examples::QuerySnapshotResponse* response)
{
    if (controller->Failed()) {
        LOG(FATAL) << "failed: " << controller->ErrorText();
    }

    delete controller;
    delete request;
    delete response;
    --g_request_count;

    if (!g_quit) {
        SendRequest(snapshot_client);
    }
}

void SendRequest(rpc_examples::SnapshotServer::Stub* snapshot_client)
{
    poppy::RpcController* rpc_controller = new poppy::RpcController();
    rpc_examples::QuerySnapshotRequest* request = new rpc_examples::QuerySnapshotRequest();
    rpc_examples::QuerySnapshotResponse* response = new rpc_examples::QuerySnapshotResponse();
    google::protobuf::Closure* done =
        NewClosure(&QueryCallback, snapshot_client, rpc_controller, request, response);
    request->set_docid(docid);
    ++g_request_count;
    snapshot_client->QuerySnapshot(rpc_controller, request, response, done);
}

bool AddSnapshot(rpc_examples::SnapshotServer::Stub* snapshot_client)
{
    std::string content;

    if (!io::textfile::LoadToString(FLAGS_data_file, &content)) {
        LOG(INFO) << "can not open data file: " << FLAGS_data_file;
        return false;
    }

    poppy::RpcController rpc_controller;
    rpc_examples::AddSnapshotRequest request;
    poppy::EmptyResponse response;
    request.set_docid(docid);
    request.set_content(content);

    LOG(INFO) << "sending add snapshot request: docid=" << request.docid()
              << ", content_len=" << request.content().length();

    snapshot_client->AddSnapshot(&rpc_controller, &request, &response, NULL);

    if (rpc_controller.Failed()) {
        LOG(INFO) << "add snapsot failed: " << rpc_controller.ErrorText();
        return false;
    } else {
        LOG(INFO) << "add snapshot success.";
    }

    return true;
}

int main(int argc, char** argv)
{
    FLAGS_v = 3;
    FLAGS_log_dir = ".";
    google::ParseCommandLineFlags(&argc, &argv, true);

    google::InitGoogleLogging(argv[0]);

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);

    rpc_examples::ConnectionManager::instance()->SetServerAddress(
        FLAGS_server_address);
    rpc_examples::SnapshotServer::Stub
    snapshot_client(rpc_examples::ConnectionManager::instance()->GetChannel());

    if (!AddSnapshot(&snapshot_client)) {
        return EXIT_FAILURE;
    }

    for (int32_t i = 0; i < FLAGS_request_number; ++i) {
        SendRequest(&snapshot_client);
    }

    for (;;) {
        ThisThread::Sleep(100);

        if (g_quit && g_request_count == 0) {
            break;
        }
    }

    LOG(INFO) << "All response is back, now quit...";

    return EXIT_SUCCESS;
}
