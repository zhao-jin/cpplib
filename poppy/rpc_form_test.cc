// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Yongsong Liu <ericliu@tencent.com>
// Created: 12/18/1
// Description:

#include <string>
#include "common/base/string/string_number.h"
#include "common/net/http/http_message.h"
#include "poppy/rpc_form.h"
#include "thirdparty/gtest/gtest.h"
#include "thirdparty/perftools/heap-checker.h"

namespace poppy {

namespace {

static void FormRequestComplete(HttpResponse* http_response)
{
    if (http_response->status() < 0) {
        http_response->set_status(HttpResponse::Status_OK);
    }
    if (!http_response->HasHeader("Content-Type")) {
        http_response->AddHeader("Content-Type", "text/html");
    }
    if (http_response->http_body().empty()) {
        http_response->mutable_http_body()->assign(
                HttpResponse::StatusCodeToReasonPhrase(http_response->status()));
    }
    if (!http_response->HasHeader("Content-Length")) {
        http_response->AddHeader("Content-Length",
                IntegerToString(http_response->http_body().size()));
    }
}

} // namespace

TEST(RpcFormTest, NormalListPage)
{
    HttpRequest http_request;
    http_request.set_uri("/rpc/form");
    http_request.set_method(HttpRequest::METHOD_GET);
    HttpResponse http_response;
    HandleFormRequest(&http_request, &http_response,
        NewClosure(FormRequestComplete, &http_response));
    EXPECT_EQ(HttpResponse::Status_OK, http_response.status());
    std::string content_type;
    EXPECT_TRUE(http_response.GetHeader("Content-Type", &content_type));
    EXPECT_EQ("text/html; charset=utf-8", content_type);
    EXPECT_TRUE(http_response.http_body().find("Poppy Methods for") != std::string::npos);
}

TEST(RpcFormTest, MethodInvalid)
{
    HttpRequest http_request;
    http_request.set_uri("/rpc/form");
    http_request.set_method(HttpRequest::METHOD_POST);
    HttpResponse http_response;
    HandleFormRequest(&http_request, &http_response,
        NewClosure(FormRequestComplete, &http_response));
    EXPECT_EQ(HttpResponse::Status_MethodNotAllowed, http_response.status());
    std::string content_type;
    EXPECT_TRUE(http_response.GetHeader("Content-Type", &content_type));
    EXPECT_EQ("text/html", content_type);
    EXPECT_EQ("Only accept get method.", http_response.http_body());
}

TEST(RpcFormTest, PathInvalid)
{
    HttpRequest http_request;
    http_request.set_uri("/rpc/form2");
    http_request.set_method(HttpRequest::METHOD_GET);
    HttpResponse http_response;
    HandleFormRequest(&http_request, &http_response,
        NewClosure(FormRequestComplete, &http_response));
    EXPECT_EQ(HttpResponse::Status_BadRequest, http_response.status());
    std::string content_type;
    EXPECT_TRUE(http_response.GetHeader("Content-Type", &content_type));
    EXPECT_EQ("text/html", content_type);
    EXPECT_EQ("Method name is invalid", http_response.http_body());
}

TEST(RpcFormTest, NormalMethodCallPage)
{
    HttpRequest http_request;
    http_request.set_uri("/rpc/form/rpc_examples.EchoServer.Echo");
    http_request.set_method(HttpRequest::METHOD_GET);
    HttpResponse http_response;
    HandleFormRequest(&http_request, &http_response,
        NewClosure(FormRequestComplete, &http_response));
    EXPECT_EQ(HttpResponse::Status_OK, http_response.status());
    std::string content_type;
    EXPECT_TRUE(http_response.GetHeader("Content-Type", &content_type));
    EXPECT_EQ("text/html; charset=utf-8", content_type);
    EXPECT_TRUE(http_response.http_body().find("Form for rpc_examples.EchoServer.Echo")
                != std::string::npos);
}

} // namespace poppy

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    HeapLeakChecker checker("Test with memory leak detection");
    // return 0(false) means OK,  so we need add a not operator
    return RUN_ALL_TESTS() || !checker.NoLeaks();
}
