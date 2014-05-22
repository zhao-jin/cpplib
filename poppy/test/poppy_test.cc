// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Dongping Huang <dphuang@tencent.com>
// Created: 06/13/11

#include "common/base/array_size.h"
#include "common/base/singleton.h"
#include "common/base/string/concat.h"
#include "common/base/string/string_number.h"
#include "common/net/http/http_downloader.h"
#include "common/system/concurrency/thread_group.h"
#include "common/system/time/stopwatch.h"
#include "poppy/test/echo_server.h"
#include "poppy/test/echo_test.h"
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/gtest/gtest.h"
#include "thirdparty/perftools/heap-checker.h"

// using namespace common;

namespace {

const int32_t kTimeoutMaxDiff = 500; // 500 milliseconds

bool TimeoutMatch(int64_t timeout, int64_t expected, int64_t max_diff)
{
    return (timeout <= expected + max_diff) && (timeout >= expected - max_diff);
}

} // namespace

int g_export_value_1 = 5;
int g_export_value_2 = 4;
EXPORT_VARIABLE(export_value_1, &g_export_value_1);
EXPORT_VARIABLE(export_value_2, &g_export_value_2);

using common::HttpDownloader;

struct HttpTestData {
    HttpRequest::MethodType method;
    std::string path;
    std::string request_content_type;
    std::string request_body;
    bool expect_success;
    HttpResponse::StatusCode expect_status_code;
    std::string expect_content_type;
    const char* expect_body;
};

class PoppyConnectionTest : public testing::Test
{
public:
    virtual void SetUp() {
        rpc_examples::EchoClient::s_crash_on_timeout = false;

        for (int i = 0; i < kServerNum; ++i) {
            m_test_server[i] = new rpc_examples::TestServer;
            m_test_server_port[i] = 0;
            StartServer(i);
        }
    }
    virtual void TearDown() {
        for (int i = 0; i < kServerNum; ++i) {
            delete m_test_server[i];
            m_test_server[i] = 0;
        }
    }

    std::string GetServerAddress(int index) {
        CheckServerIndex(index);

        std::string ip = "127.0.0.1";
        return StringConcat(ip, ":", m_test_server_port[index]);
    }

    bool StartServer(int index, common::CredentialVerifier* credential_verifier = NULL) {
        CheckServerIndex(index);

        if (!m_test_server[index]) {
            m_test_server[index] = new rpc_examples::TestServer(0, credential_verifier);
        }

        SocketAddressInet4 real_bind_address;
        m_test_server[index]->Start(GetServerAddress(index), &real_bind_address);
        m_test_server_port[index] = real_bind_address.GetPort();
        return true;
    }

    bool StopServer(int index) {
        CheckServerIndex(index);

        delete m_test_server[index];
        m_test_server[index] = 0;
        return true;
    }

    bool EnableServer(int index) {
        CheckServerIndex(index);

        if (!m_test_server[index]) {
            return false;
        }

        m_test_server[index]->SetHealthy(true);
        return true;
    }

    bool DisableServer(int index) {
        CheckServerIndex(index);

        if (!m_test_server[index]) {
            return false;
        }

        m_test_server[index]->SetHealthy(false);
        return true;
    }

    void CheckServerIndex(int index)
    {
        CHECK(index >= 0 && index < kServerNum)
            << " server index out of range,"
            << "make sure it's greater than 0 and less than "
            << kServerNum;
    }

    void DoSendRequests(bool use_channel_cache)
    {
        scoped_ptr<rpc_examples::EchoClient> client;
        if (use_channel_cache) {
            client.reset(new rpc_examples::EchoClient(GetServerAddress(0)));
        } else {
            poppy::RpcChannelOptions options;
            options.set_use_channel_cache(false);
            client.reset(new rpc_examples::EchoClient(GetServerAddress(0), options));
        }
        client->RunAsyncTest(100, 0, false);
        client->Shutdown(true);
    }

    void DoSendRequestsShareInstance(bool use_channel_cache)
    {
        scoped_ptr<rpc_examples::EchoClient> client;
        if (use_channel_cache) {
            client.reset(new rpc_examples::EchoClient(GetServerAddress(0),
                                                      poppy::RpcChannelOptions(),
                                                      true));
        } else {
            poppy::RpcChannelOptions options;
            options.set_use_channel_cache(false);
            client.reset(new rpc_examples::EchoClient(GetServerAddress(0), options, true));
        }
        client->RunAsyncTest(100, 0, false);
    }

    void DoHttpRequest(const HttpTestData& request)
    {
        if (request.method != HttpRequest::METHOD_GET &&
            request.method != HttpRequest::METHOD_POST)
            return;

        HttpDownloader downloader;
        HttpResponse response;
        HttpDownloader::ErrorType error;

        HttpDownloader::Options options;
        if (request.method == HttpRequest::METHOD_POST)
            options.Headers().Add("Content-Type", request.request_content_type);

        bool ret;
        if (request.method == HttpRequest::METHOD_GET) {
            ret = downloader.Get("http://" + GetServerAddress(0) + request.path,
                                 options,
                                 &response,
                                 &error);
        } else if (request.method == HttpRequest::METHOD_POST) {
            options.Headers().Add("Content-Length", NumberToString(request.request_body.size()));
            ret = downloader.Post("http://" + GetServerAddress(0) + request.path,
                                  request.request_body,
                                  options,
                                  &response,
                                  &error);
        }

        if (request.expect_success)
            EXPECT_EQ(HttpDownloader::SUCCESS, error);

        EXPECT_EQ(request.expect_success, ret);
        EXPECT_EQ(request.expect_status_code, response.status());

        std::string content_type;
        EXPECT_TRUE(response.GetHeader("Content-Type", &content_type));
        EXPECT_EQ(request.expect_content_type, content_type);
        if (request.expect_body != NULL)
            EXPECT_EQ(request.expect_body, response.http_body());
    }


    static const int kServerNum = 4;
    rpc_examples::TestServer* m_test_server[kServerNum];
    uint16_t m_test_server_port[kServerNum];
};

TEST_F(PoppyConnectionTest, Empty)
{
}

TEST_F(PoppyConnectionTest, Simple)
{
    rpc_examples::EchoClient client(StringConcat(GetServerAddress(0), ",",
                                                 GetServerAddress(1), ",",
                                                 GetServerAddress(2), ",",
                                                 GetServerAddress(3)));
    // The following call to TearDown() is to stop test poppy server before
    // the client start to Shutdown.
    // This is used to try to simulate the completion when Shutdown RpcClient.
    // Without the TearDown call, poppy server will be closed after RpcClient.
    TearDown();
}

TEST_F(PoppyConnectionTest, BasicSingleServer)
{
    rpc_examples::EchoClient client(GetServerAddress(0));
    int throughput = 0;

    throughput = client.RunLatencyTest(10);
    VLOG(3) << "Latency test: " << throughput << "/s\n";

    throughput = client.RunReadTest(10);
    VLOG(3) << "Read throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    throughput = client.RunWriteTest(10);
    VLOG(3) << "Write throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    throughput = client.RunReadWriteTest(10);
    VLOG(3) << "Read/Write throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    client.RunAsyncTest(10);
}

TEST_F(PoppyConnectionTest, BasicMultipleServer)
{
    rpc_examples::EchoClient client(StringConcat(GetServerAddress(0), ",",
                                                 GetServerAddress(1), ",",
                                                 GetServerAddress(2), ",",
                                                 GetServerAddress(3)));
    int throughput = 0;

    throughput = client.RunLatencyTest(10);
    VLOG(3) << "Latency test: " << throughput << "/s\n";

    throughput = client.RunReadTest(10);
    VLOG(3) << "Read throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    throughput = client.RunWriteTest(10);
    VLOG(3) << "Write throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    throughput = client.RunReadWriteTest(10);
    VLOG(3) << "Read/Write throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    client.RunAsyncTest(10);
}

TEST_F(PoppyConnectionTest, DynamicAddress)
{
    rpc_examples::EchoClient client(GetServerAddress(0));
    int throughput = 0;

    throughput = client.RunLatencyTest(10);
    VLOG(3) << "Latency test: " << throughput << "/s\n";

    throughput = client.RunReadTest(500);
    VLOG(3) << "Read throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    client.ChangeAddresses(GetServerAddress(1));

    throughput = client.RunWriteTest(50);
    VLOG(3) << "Write throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    throughput = client.RunReadWriteTest(50);
    VLOG(3) << "Read/Write throughput: " << throughput << "M/s\n";
    EXPECT_GT(throughput, 0) << "Error: Package lost.";

    client.RunAsyncTest(10);
}

TEST_F(PoppyConnectionTest, ConnectTimeoutLessThanServiceTimeout)
{
    Stopwatch sw;
    poppy::RpcChannelOptions options;
    options.set_connect_timeout(1000);

    rpc_examples::EchoClient client(GetServerAddress(0), options);
    StopServer(0);
    sw.Restart();
    client.RunAsyncTest(10);
    sw.Stop();
    // connect timeout is connect_timeout = 1000 ms
    // service timeout is 2000 ms
    // so it will return in connect timeout
    VLOG(3) << "Elapsed time, close to 1000 ms: " << sw.ElapsedMilliSeconds();
    EXPECT_PRED3(TimeoutMatch, sw.ElapsedMilliSeconds(),
                 options.connect_timeout(), kTimeoutMaxDiff);
}

TEST_F(PoppyConnectionTest, ConnectTimeoutLargerThanServiceTimeout)
{
    Stopwatch sw;
    poppy::RpcChannelOptions options;
    options.set_connect_timeout(3000);

    rpc_examples::EchoClient client(GetServerAddress(0), options);
    StopServer(0);
    sw.Restart();
    client.RunAsyncTest(10);
    sw.Stop();
    // connect timeout is connect_timeout = 3000 ms
    // service timeout is 2000 ms
    // so it will return in service timeout
    VLOG(3) << "Elapsed time, close to 2000 ms: " << sw.ElapsedMilliSeconds();
    EXPECT_PRED3(TimeoutMatch, sw.ElapsedMilliSeconds(),
                 2000, kTimeoutMaxDiff);
}

TEST_F(PoppyConnectionTest, ServiceTimeout)
{
    Stopwatch sw;
    poppy::RpcChannelOptions options;
    options.set_connect_timeout(1000);

    rpc_examples::EchoClient client(GetServerAddress(0), options);
    sw.Restart();
    client.RunAsyncTest(1, 5000);
    sw.Stop();
    // Because default service timeout is 2000 ms,
    // async test timeout = 5000 ms
    // so it will return in 2000 ms
    VLOG(3) << "Elapsed time, close to 2000 ms: " << sw.ElapsedMilliSeconds();
    EXPECT_PRED3(TimeoutMatch, sw.ElapsedMilliSeconds(),
                 2000, kTimeoutMaxDiff);
    sw.Start();
    StopServer(0);
    sw.Stop();
    // wait for all completion request finish, ensure server can stop normally
    VLOG(3) << "Elapsed time, close to 5000 ms: " << sw.ElapsedMilliSeconds();
    EXPECT_PRED3(TimeoutMatch, sw.ElapsedMilliSeconds(),
                 5000, kTimeoutMaxDiff);
}

TEST_F(PoppyConnectionTest, ConnectTimeout)
{
    Stopwatch sw;
    poppy::RpcChannelOptions options;
    options.set_connect_timeout(1000);

    rpc_examples::EchoClient client(GetServerAddress(0), options);
    sw.Restart();
    client.RunAsyncTest(1, 1000);
    sw.Stop();
    // should be async test timeout = 1000 ms
    VLOG(3) << "Elapsed time, close to 1000 ms: " << sw.ElapsedMilliSeconds();
    EXPECT_PRED3(TimeoutMatch, sw.ElapsedMilliSeconds(),
                 1000, kTimeoutMaxDiff);
    StopServer(0);
    sw.Restart();
    client.RunAsyncTest(1);
    sw.Stop();
    // should be ChannelImpl's connection timeout = 1000 ms
    VLOG(3) << "Elapsed time, close to 1000 ms: " << sw.ElapsedMilliSeconds();
    EXPECT_PRED3(TimeoutMatch, sw.ElapsedMilliSeconds(),
                 1000, kTimeoutMaxDiff);
}

TEST_F(PoppyConnectionTest, TimeoutFirstHalf)
{
    rpc_examples::EchoClient client(GetServerAddress(0));
    StopServer(0);
    client.SendAsyncTestRequest(50);
    StartServer(0);
    client.WaitForAsyncTestFinish();
    VLOG(3) << client.Dump();
}

TEST_F(PoppyConnectionTest, AsyncWithFailureServer)
{
    std::string server_address = StringConcat(GetServerAddress(0), ",",
                                              GetServerAddress(1));
    rpc_examples::EchoClient client(server_address);

    client.SendAsyncTestRequest(50);
    StopServer(0);
    client.WaitForAsyncTestFinish();
    client.SendAsyncTestRequest(50);
    StopServer(1);
    StartServer(0);
    client.WaitForAsyncTestFinish();
}

TEST_F(PoppyConnectionTest, DeleteChannel)
{
    std::string server_address = GetServerAddress(0);
    rpc_examples::EchoClient* client =
        new rpc_examples::EchoClient(server_address);

    client->SendAsyncTestRequest(50);
    client->Shutdown(true);
    delete client;
}

TEST_F(PoppyConnectionTest, Healthy)
{
    rpc_examples::EchoClient client(GetServerAddress(0));
    rpc_examples::RpcChannel* channel = client.GetChannel();

    while (channel->GetChannelStatus() != poppy::ChannelStatus_Healthy) {
        VLOG(3) << "Wait channel status to be Healthy\n";
        ThisThread::Sleep(10);
    }

    EXPECT_EQ(channel->GetChannelStatus(), ::poppy::ChannelStatus_Healthy);

    // normal case.
    client.SendAsyncTestRequest(10);
    client.WaitForAsyncTestFinish();

    // set server to not healthy, following 5 packages should fail.
    DisableServer(0);
    ThisThread::Sleep(1200);

    EXPECT_EQ(channel->GetChannelStatus(), ::poppy::ChannelStatus_Unavailable);

    client.SendAsyncTestRequest(2);
    client.WaitForAsyncTestFinish();

    // send the request, and enable the server later, these 5 request should be OK.
    client.SendAsyncTestRequest(2);
    EnableServer(0);
    client.WaitForAsyncTestFinish();

    ThisThread::Sleep(1200);
    EXPECT_EQ(channel->GetChannelStatus(), ::poppy::ChannelStatus_Healthy);

    client.SendAsyncTestRequest(2);
    client.WaitForAsyncTestFinish();
}

TEST_F(PoppyConnectionTest, RejectVerify)
{
    rpc_examples::EchoClient client(GetServerAddress(0));
    StopServer(0);
    StartServer(0, &Singleton<rpc_examples::RejectCredentialVerifier>::Instance());
    client.RunAsyncTest(5);
}

TEST_F(PoppyConnectionTest, Keepalive)
{
    poppy::RpcChannelOptions options;
    options.set_keepalive_time(1000);
    rpc_examples::EchoClient client(GetServerAddress(0), options);

    for (int k = 0; k < 10; ++k) {
        client.RunLatencyTest(2);
        ThisThread::Sleep(1000);
    }
}

TEST_F(PoppyConnectionTest, ReconnectAfterIdle)
{
    poppy::RpcChannelOptions options;
    options.set_keepalive_time(1000);
    rpc_examples::EchoClient client(GetServerAddress(0), options);

    for (int k = 0; k < 2; ++k) {
        client.RunLatencyTest(2);
        ThisThread::Sleep(1100);
        client.RunLatencyTest(2);
        ThisThread::Sleep(1100);
    }
}

TEST_F(PoppyConnectionTest, ShutdownWithConnectionInCache)
{
    poppy::RpcClient client;
    {
        poppy::RpcChannel channel(&client, GetServerAddress(0));
    }
    {
        poppy::RpcChannel channel(&client, GetServerAddress(1));
    }
}

TEST_F(PoppyConnectionTest, MultiRpcClientShareCache)
{
    {
        // use channel cache
        ThreadGroup threads(Bind(&PoppyConnectionTest::DoSendRequests, this, true), 10);
        threads.Start();
        threads.Join();
    }

    {
        // not use channel cache
        ThreadGroup threads(Bind(&PoppyConnectionTest::DoSendRequests, this, false), 10);
        threads.Start();
        threads.Join();
    }
}

TEST_F(PoppyConnectionTest, MultiRpcClientShareInstance)
{
    {
        // use channel cache
        ThreadGroup threads(Bind(&PoppyConnectionTest::DoSendRequestsShareInstance, this, true),
                            10);
        threads.Start();
        threads.Join();
    }

    {
        // not use channel cache
        ThreadGroup threads(Bind(&PoppyConnectionTest::DoSendRequestsShareInstance, this, false),
                            10);
        threads.Start();
        threads.Join();
    }
}

TEST_F(PoppyConnectionTest, TextInterface)
{
    static const char* kValidJsonText = "{\"message\":\"hello,中国人\",\"user\":\"ericliu\"}\n";
    static const std::string kJsonContentType = "application/json; charset=utf-8";
    static const std::string kXProtoContentType = "text/x-proto; charset=utf-8";
    static const std::string kHtmlContentType = "text/html";

    static const HttpTestData requests[] = {
        {
            HttpRequest::METHOD_POST,
            "/rpc/rpc_examples.EchoServer.Echo",
            kJsonContentType,
            kValidJsonText,
            true,
            HttpResponse::Status_OK,
            kJsonContentType,
            kValidJsonText
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/rpc_examples.EchoServer.Echo",
            kXProtoContentType,
            "message: \"hello,中国人\"\nuser: \"ericliu\"\n",
            true,
            HttpResponse::Status_OK,
            kXProtoContentType,
            "user: \"ericliu\"\nmessage: \"hello,中国人\"\n"
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/rpc_examples.EchoServer.Echo?ie=gbk",
            kJsonContentType,
            "{\"message\": \"hello,\xEA\xEA\", \"user\": \"ericliu\"}",
            false,
            HttpResponse::Status_BadRequest,
            kHtmlContentType,
            "Failed to convert request from utf-8 to gbk\n"
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/rpc_examples.EchoServer.Echo?oe=gbk",
            kJsonContentType,
            kValidJsonText,
            false,
            HttpResponse::Status_BadRequest,
            kHtmlContentType,
            NULL
        }, {
            HttpRequest::METHOD_GET,
            "/rpc/rpc_examples.EchoServer.Echo",
            kJsonContentType,
            "",
            false,
            HttpResponse::Status_MethodNotAllowed,
            kHtmlContentType,
            "Only accept POST method"
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/rpc_examples.EchoServer.Echo",
            "",
            "",
            false,
            HttpResponse::Status_BadRequest,
            kHtmlContentType,
            "Request Content-Type is missed or invalid"
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/",
            kJsonContentType,
            kValidJsonText,
            false,
            HttpResponse::Status_BadRequest,
            kHtmlContentType,
            "Method full name is invalid"
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/rpc_examples2.EchoServer.Echo",
            kJsonContentType,
            kValidJsonText,
            false,
            HttpResponse::Status_NotFound,
            kHtmlContentType,
            "Service not found"
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/rpc_examples.EchoServer.Echo2",
            kJsonContentType,
            kValidJsonText,
            false,
            HttpResponse::Status_NotFound,
            kHtmlContentType,
            "Method not found"
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/rpc_examples.EchoServer.Echo",
            "application/son; charset=utf-8",
            kValidJsonText,
            false,
            HttpResponse::Status_BadRequest,
            kHtmlContentType,
            "Not support request Content-Type"
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/rpc_examples.EchoServer.Echo?of=x",
            kJsonContentType,
            kValidJsonText,
            false,
            HttpResponse::Status_BadRequest,
            kHtmlContentType,
            "Not support response Content-Type"
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/rpc_examples.EchoServer.Echo",
            kJsonContentType,
            "{message\": \"hello,中国人\", \"user\": \"ericliu\"}",
            false,
            HttpResponse::Status_BadRequest,
            kHtmlContentType,
            NULL
        }
    };

    for (size_t i = 0; i < ARRAY_SIZE(requests); i++) {
        DoHttpRequest(requests[i]);
    }
}

TEST_F(PoppyConnectionTest, BuiltinService)
{
    static const std::string kJsonContentType = "application/json; charset=utf-8";
    static const std::string kHtmlContentType = "text/html";

    static const HttpTestData requests[] = {
        {
            HttpRequest::METHOD_POST,
            "/rpc/poppy.BuiltinService.Health?of=json",
            kJsonContentType,
            "{}",
            true,
            HttpResponse::Status_OK,
            kJsonContentType,
            "{\"health\":\"OK\"}\n"
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/poppy.BuiltinService.GetAllServiceDescriptors?of=json",
            kJsonContentType,
            "{}",
            true,
            HttpResponse::Status_OK,
            kJsonContentType,
            "{\"services\":[{\"methods\":[{\"name\":\"Echo\"},{\"name\":\"Get\"},"
            "{\"name\":\"Set\"}],\"name\":\"rpc_examples.EchoServer\"}]}\n"
        }, {
            HttpRequest::METHOD_POST,
            "/rpc/poppy.BuiltinService.GetMethodInputTypeDescriptors?of=json",
            kJsonContentType,
            "{\"method_full_name\": \"rpc_examples.EchoServer.Echo\"}",
            true,
            HttpResponse::Status_OK,
            kJsonContentType,
            "{\"enum_types\":[{\"name\":\"rpc_examples.TestEnum\",\"values\":[{\"name\":\"V1\","
            "\"number\":1},{\"name\":\"V2\",\"number\":2}]}],\"message_types\":[{\"fields\":[{"
            "\"default_value\":\"test ext\",\"label\":1,\"name\":\"query_ext\",\"number\":100,"
            "\"type\":9},{\"label\":1,\"name\":\"long_ext\",\"number\":101,\"type\":4},{\"label\":"
            "2,\"name\":\"user\",\"number\":1,\"type\":9},{\"label\":2,\"name\":\"message\",\"numb"
            "er\":2,\"type\":9},{\"label\":1,\"name\":\"timeout\",\"number\":3,\"type\":1},{\"labe"
            "l\":1,\"name\":\"flag\",\"number\":4,\"type\":7},{\"default_value\":\"1\",\"label\":1"
            ",\"name\":\"test_enum\",\"number\":5,\"type\":8,\"type_name\":\"rpc_examples.TestEnum"
            "\"},{\"label\":1,\"name\":\"nested_msg\",\"number\":6,\"type\":10,\"type_name\":\"rpc"
            "_examples.NestedMsg\"},{\"label\":3,\"name\":\"nested_msgs\",\"number\":7,\"type\":10"
            ",\"type_name\":\"rpc_examples.NestedMsg\"},{\"label\":3,\"name\":\"rep_int\",\"number"
            "\":8,\"type\":1},{\"label\":1,\"name\":\"b\",\"number\":9,\"type\":11},{\"label\":3,"
            "\"name\":\"bs\",\"number\":10,\"type\":11}],\"name\":\"rpc_examples.EchoRequest\"},{"
            "\"fields\":[{\"label\":1,\"name\":\"id\",\"number\":1,\"type\":1},{\"label\":1,\"name"
            "\":\"title\",\"number\":2,\"type\":9},{\"label\":1,\"name\":\"url\",\"number\":3,\"ty"
            "pe\":9}],\"name\":\"rpc_examples.NestedMsg\"}],\"request_type\":\"rpc_examples.EchoRe"
            "quest\"}\n"
        }
    };

    for (size_t i = 0; i < ARRAY_SIZE(requests); i++) {
        DoHttpRequest(requests[i]);
    }
}

TEST_F(PoppyConnectionTest, ExportVariables)
{
    static const std::string kJsonContentType = "application/json";
    static const std::string kTextContentType = "text/plain";

    static const HttpTestData requests[] = {
        {
            HttpRequest::METHOD_POST,
            "/vars",
            kJsonContentType,
            "",
            true,
            HttpResponse::Status_OK,
            kTextContentType,
            NULL
        },
        {
            HttpRequest::METHOD_POST,
            "/vars?filter=export_value_1",
            kJsonContentType,
            "",
            true,
            HttpResponse::Status_OK,
            kTextContentType,
            "export_value_1 = 5\n"
        },
        {
            HttpRequest::METHOD_POST,
            "/vars?filter=export_value_1,export_value_2",
            kJsonContentType,
            "",
            true,
            HttpResponse::Status_OK,
            kTextContentType,
            "export_value_1 = 5\nexport_value_2 = 4\n"
        },
        {
            HttpRequest::METHOD_POST,
            "/vars?filter=export_value_1,export_value_3",
            kJsonContentType,
            "",
            true,
            HttpResponse::Status_OK,
            kTextContentType,
            "export_value_1 = 5\nexport_value_3 =  \n"
        },
        {
            HttpRequest::METHOD_POST,
            "/vars?of=json",
            kJsonContentType,
            "",
            true,
            HttpResponse::Status_OK,
            kJsonContentType,
            NULL
        },
        {
            HttpRequest::METHOD_POST,
            "/vars?of=json&filter=export_value_1",
            kJsonContentType,
            "",
            true,
            HttpResponse::Status_OK,
            kJsonContentType,
            "{\"export_value_1\":\"5\"}\n"
        },
        {
            HttpRequest::METHOD_POST,
            "/vars?of=json&filter=export_value_1,export_value_2",
            kJsonContentType,
            "",
            true,
            HttpResponse::Status_OK,
            kJsonContentType,
            "{\"export_value_1\":\"5\",\"export_value_2\":\"4\"}\n"
        },
        {
            HttpRequest::METHOD_POST,
            "/vars?of=json&filter=export_value_1,export_value_3",
            kJsonContentType,
            "",
            true,
            HttpResponse::Status_OK,
            kJsonContentType,
            "{\"export_value_1\":\"5\",\"export_value_3\":\" \"}\n"
        }
    };

    for (size_t i = 0; i < ARRAY_SIZE(requests); i++) {
        DoHttpRequest(requests[i]);
    }
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    HeapLeakChecker checker("Test with memory leak detection");
    // return 0(false) means OK,  so we need add a not operator
    return RUN_ALL_TESTS() || !checker.NoLeaks();
}

