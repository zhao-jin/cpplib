// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Eric Liu <ericliu@tencent.com>
// Created: 07/19/11
// Description: test protobuf message json serializer

#include <string>
#include "common/encoding/json_to_pb.h"
#include "common/encoding/pb_to_json.h"
#include "common/system/time/timestamp.h"
#include "glog/logging.h"
#include "gtest/gtest.h"
#include "poppy/examples/echo_sync/echo_service.pb.h"

using namespace std;
using namespace google::protobuf;
// using namespace common;

class PbJsonTest : public ::testing::Test
{
public:
    virtual void SetUp();

protected:
    string kJsonString;
};

void PbJsonTest::SetUp()
{
    kJsonString.assign(
        "{\"message\":\"hello%2C%E4%B8%AD%E5%9B%BD%E4%BA%BA\","
        "\"user\":\"ericliu\"}");
}

TEST_F(PbJsonTest, ProtoMessageToJsonSpeed)
{
    rpc_examples::EchoResponse test_message;
    test_message.set_user("ericliu");
    test_message.set_message(
        "echo+from+server%3A+127.0.0.1%3A10000%2C+"
        "message%3A+hello%2C%E4%B8%AD%E5%9B%BD%E4%BA%BA");

    string json_string;
    uint32_t loop_count = 100000;
    uint64_t t = GetTimeStampInUs();

    for (uint32_t i = 0; i < loop_count; i++) {
        ASSERT_TRUE(ProtoMessageToJson(test_message, &json_string));
    }

    t = GetTimeStampInUs() - t;
    uint64_t throughput = loop_count / (t / 1000000.0);
    LOG(INFO) << "ProtoMessage To Json Convert Speed = " << throughput << " /s";
}

TEST_F(PbJsonTest, JsonToProtoMessageSpeed)
{
    StringPiece json_data(kJsonString);

    rpc_examples::EchoRequest test_message;
    uint32_t loop_count = 100000;
    uint64_t t = GetTimeStampInUs();

    for (uint32_t i = 0; i < loop_count; i++) {
        ASSERT_TRUE(JsonToProtoMessage(json_data, &test_message));
    }

    t = GetTimeStampInUs() - t;
    uint64_t throughput = loop_count / (t / 1000000.0);
    LOG(INFO) << "Json To ProtoMessage Convert Speed = " << throughput << " /s";
}
