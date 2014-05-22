// Copyright 2011, Tencent Inc.
// Author: Eric Liu <ericliu@tencent.com>
//
// An example snapshot client.

#include <string>
#include "common/base/string/algorithm.h"

// rpc client related headers.
#include "poppy/examples/snapshot/snapshot_service.pb.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"

// read data file
#include "common/system/io/textfile.h"

// includes from thirdparty
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

// using namespace common;

DEFINE_string(server_address, "127.0.0.1:10000", "the address server listen on.");
DEFINE_string(data_file, "qq.dat", "the snapshot data file path.");

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    // Define an rpc client. It's shared by all rpc channels.
    poppy::RpcClient rpc_client;

    // Define an rpc channel. It connects to a specified group of servers.
    poppy::RpcChannel rpc_channel(&rpc_client, FLAGS_server_address);

    // Define an rpc stub. It use a specifed channel to send requests.
    rpc_examples::SnapshotServer::Stub snapshot_client(&rpc_channel);

    uint64_t docid = 12298956196516809855ULL;

    // test block
    {
        std::string content;

        if (!io::textfile::LoadToString(FLAGS_data_file, &content)) {
            LOG(INFO) << "can not open data file: " << FLAGS_data_file;
            return EXIT_FAILURE;
        }

        poppy::RpcController rpc_controller;
        rpc_examples::AddSnapshotRequest request;
        poppy::EmptyResponse response;
        request.set_docid(docid);
        request.set_content(content);

        LOG(INFO) << "sending add snapshot request: docid=" << request.docid()
                  << ", content_len=" << request.content().length();

        snapshot_client.AddSnapshot(&rpc_controller, &request, &response, NULL);

        if (rpc_controller.Failed()) {
            LOG(INFO) << "add snapsot failed: " << rpc_controller.ErrorText();
        } else {
            LOG(INFO) << "add snapshot success.";
        }
    }

    // test block
    {
        poppy::RpcController rpc_controller;
        rpc_examples::QuerySnapshotRequest request;
        rpc_examples::QuerySnapshotResponse response;
        request.set_docid(docid);

        LOG(INFO) << "sending query snapshot request: docid=" << request.docid();

        snapshot_client.QuerySnapshot(&rpc_controller, &request, &response, NULL);

        if (rpc_controller.Failed()) {
            LOG(INFO) << "query snapsot failed: " << rpc_controller.ErrorText();
        } else {
            LOG(INFO) << "query snapsot success: content_len=" << response.content().length();
        }
    }

    return EXIT_SUCCESS;
}
