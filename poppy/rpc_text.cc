// Copyright 2011, Tencent Inc.
// Author: Eric Liu <ericliu@tencent.com>

#include "poppy/rpc_text.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "common/base/scoped_ptr.h"
#include "common/base/singleton.h"
#include "common/base/string/algorithm.h"
#include "common/base/string/concat.h"
#include "common/encoding/charset_converter.h"
#include "common/encoding/json_to_pb.h"
#include "common/encoding/pb_to_json.h"
#include "common/net/http/http_message.h"
#include "common/net/uri/cgi_params.h"
#include "common/net/uri/uri.h"

#include "poppy/rpc_handler.h"
#include "poppy/rpc_server.h"

#include "thirdparty/glog/logging.h"
#include "thirdparty/protobuf/descriptor.h"
#include "thirdparty/protobuf/text_format.h"

namespace poppy {

// using namespace common;

class TextConverter
{
public:
    explicit TextConverter(const std::string& mime) : m_mime(mime) {}
    virtual ~TextConverter() {}
    virtual const std::string& Mime() const
    {
        return m_mime;
    }

    virtual bool Serialize(const google::protobuf::Message& message,
                           std::string* output,
                           bool use_beautiful_text_format,
                           bool use_utf8_string_escaping = true) = 0;
    virtual bool Deserialize(const std::string& input,
                             google::protobuf::Message* message,
                             std::string* error,
                             bool urlencoded) = 0;
protected:
    std::string m_mime;
};

namespace {

class JsonTextConverter : public TextConverter
{
public:
    explicit JsonTextConverter(const std::string& mime) : TextConverter(mime) {}

    bool Serialize(const google::protobuf::Message& message,
                   std::string* output,
                   bool use_beautiful_text_format,
                   bool use_utf8_string_escaping)
    {
        return ProtoMessageToJson(message, output, NULL, use_beautiful_text_format);
    }

    bool Deserialize(const std::string& input,
                     google::protobuf::Message* message,
                     std::string* error,
                     bool urlencoded)
    {
        return JsonToProtoMessage(input, message, error, urlencoded);
    }
};

class ProtobufTextConverter : public TextConverter
{
public:
    explicit ProtobufTextConverter(const std::string& mime) : TextConverter(mime) {}

    bool Serialize(const google::protobuf::Message& message,
                   std::string* output,
                   bool use_beautiful_text_format,
                   bool use_utf8_string_escaping)
    {
        google::protobuf::TextFormat::Printer printer;
        printer.SetUseUtf8StringEscaping(use_utf8_string_escaping);
        if (use_beautiful_text_format) {
            printer.SetUseShortRepeatedPrimitives(true);
        }
        return printer.PrintToString(message, output);
    }

    bool Deserialize(const std::string& input,
                     google::protobuf::Message* message,
                     std::string* error,
                     bool urlencoded)
    {
        error->clear();
        return google::protobuf::TextFormat::ParseFromString(input, message);
    }
};

class TextConverterMap
{
public:
    TextConverterMap()
    {
        Insert<JsonTextConverter>("json", "application/json");
        Insert<ProtobufTextConverter>("x-proto", "text/x-proto");
    }

    ~TextConverterMap()
    {
        std::set<TextConverter*>::iterator it;
        for (it = m_converter_set.begin(); it != m_converter_set.end(); ++it) {
            delete *it;
        }
    }

    TextConverter* FindConverter(const std::string& name)
    {
        std::map<std::string, TextConverter*>::iterator it = m_converter_map.find(name);
        if (it != m_converter_map.end())
            return it->second;
        else
            return NULL;
    }

private:
    template <typename T>
    void Insert(const char* name, const char* mime)
    {
        T* ptr = new T(mime);
        m_converter_set.insert(ptr);
        m_converter_map.insert(std::make_pair(name, ptr));
        m_converter_map.insert(std::make_pair(mime, ptr));
    }

private:
    std::map<std::string, TextConverter*> m_converter_map;
    std::set<TextConverter*> m_converter_set;
};

static bool GetRequestMethodInfo(
    const RpcServerHandler* rpc_handler,
    const HttpRequest* http_request,
    HttpResponse* http_response,
    google::protobuf::Service** service,
    const google::protobuf::MethodDescriptor** method)
{
    if (http_request->method() != HttpRequest::METHOD_POST) {
        // Only accept post method.
        http_response->set_status(405);
        http_response->set_body("Only accept POST method");
        return false;
    }

    std::string service_name;
    std::string method_name;
    bool ret = ParseMethodFullNameFromPath(http_request->uri(),
                                           kRpcServiceHttpPath, &service_name, &method_name);
    if (!ret || service_name.empty() || method_name.empty()) {
        LOG(ERROR) << "Method full name is invalid, request path is " << http_request->uri();
        http_response->set_status(400);
        http_response->set_body("Method full name is invalid");
        return false;
    }

    *service = rpc_handler->FindServiceByName(service_name);
    if (*service == NULL) {
        LOG(ERROR) << "Service not found, service name is " << service_name;
        http_response->set_status(404);
        http_response->set_body("Service not found");
        return false;
    }

    *method = (*service)->GetDescriptor()->FindMethodByName(method_name);
    if (*method == NULL) {
        LOG(ERROR) << "Method not found, method name is " << method_name;
        http_response->set_status(404);
        http_response->set_body("Method not found");
        return false;
    }

    return true;
}

static bool ParseRequestMessage(const HttpRequest* http_request,
                                HttpResponse* http_response,
                                google::protobuf::Message* request_message,
                                TextConverter** output_converter)
{
    std::string content_type_header;
    std::string input_format;
    if (http_request->GetHeader("Content-Type", &content_type_header)) {
        std::vector<std::string> vec;
        SplitStringByAnyOf(content_type_header, "; ", &vec);
        if (vec.empty()) {
            LOG(ERROR) << "Request Content-Type is missed or invalid";
            http_response->set_status(400);
            http_response->set_body("Request Content-Type is missed or invalid");
            return false;
        }
        input_format = vec[0];
    } else {
        input_format = "application/json";
    }

    TextConverterMap& converter_map = Singleton<TextConverterMap>::Instance();
    TextConverter* input_converter = converter_map.FindConverter(input_format);
    if (!input_converter) {
        LOG(ERROR) << "Not support request Content-Type";
        http_response->set_status(400);
        http_response->set_body("Not support request Content-Type");
        return false;
    }

    CgiParams parms;
    parms.ParseFromUrl(http_request->uri());
    std::string output_format;
    if (parms.GetValue("of", &output_format)) {
        *output_converter = converter_map.FindConverter(output_format);
        if (!(*output_converter)) {
            LOG(ERROR) << "Not support response Content-Type";
            http_response->set_status(400);
            http_response->set_body("Not support response Content-Type");
            return false;
        }
    } else {
        *output_converter = input_converter;
    }

    std::string request_encoding;
    if (!parms.GetValue("ie", &request_encoding)) {
        request_encoding = "utf-8";
    }

    std::string http_body;
    if (request_encoding != "utf-8") {
        CharsetConverter converter;
        size_t size;
        if (!converter.Create("utf-8", request_encoding) ||
            !converter.Convert(http_request->http_body(), &http_body, &size)) {
            http_response->set_status(400);
            http_response->set_body(StringConcat(
                    "Failed to convert request from utf-8 to ", request_encoding,
                    "\n"));
            LOG(ERROR) << "Failed to convert request from utf-8 to " << request_encoding;
            return false;
        }
    } else {
        http_body = http_request->http_body();
    }

    std::string param_e;
    parms.GetValue("e", &param_e);
    bool urlencoded = (param_e == "1");
    std::string error;
    if (!input_converter->Deserialize(http_body, request_message, &error, urlencoded)) {
        LOG(ERROR) << "Failed deserialize from request text\n" << error;
        http_response->set_status(400);
        http_response->set_body("Failed deserialize from request text\n" + error);
        return false;
    }

    return true;
}

static bool SerializeResponse(const HttpRequest* http_request,
                              HttpResponse* http_response,
                              TextConverter* output_converter,
                              google::protobuf::Message* response)
{
    CgiParams parms;
    parms.ParseFromUrl(http_request->uri());

    std::string response_encoding;
    if (!parms.GetValue("oe", &response_encoding)) {
        response_encoding = "utf-8";
    }

    bool use_beautiful_text_format = false;
    std::string use_beautiful;
    if (parms.GetValue("b", &use_beautiful) && use_beautiful == "1") {
        use_beautiful_text_format = true;
    }

    bool use_utf8_string_escaping = true;
    std::string use_utf8;
    if (parms.GetValue("u", &use_utf8) && use_utf8 == "0") {
        use_utf8_string_escaping = false;
    }

    if (response_encoding != "utf-8") {
        std::string tmp_string = response->Utf8DebugString();
        CharsetConverter converter;
        std::string utf8_string;
        size_t size;
        if (!converter.Create(response_encoding, "utf-8") ||
            !converter.Convert(tmp_string, &utf8_string, &size)) {
            http_response->set_status(400);
            http_response->set_body(
                StringConcat("Failed to convert response from ",
                             response_encoding, " to utf-8 encoding"));
            LOG(ERROR) << http_response->http_body();
            return false;
        }

        if (!google::protobuf::TextFormat::ParseFromString(
                utf8_string, response)) {
            http_response->set_status(400);
            http_response->set_body(
                "Failed to parse response frome utf8 string");
            LOG(ERROR) << http_response->http_body();
            return false;
        }
    }

    if (output_converter->Serialize(*response, http_response->mutable_http_body(),
                                    use_beautiful_text_format,
                                    use_utf8_string_escaping)) {
        http_response->SetHeader("Content-Type",
                                 output_converter->Mime() + "; charset=utf-8");
        return true;
    } else {
        LOG(ERROR) << "Failed serialize response to text";
        http_response->set_status(400);
        http_response->set_body("Failed serialize response to text");
        return false;
    }
}

} // namespace

void TextRequestComplete(
    int64_t begin_time,
    RpcServer* rpc_server,
    const google::protobuf::MethodDescriptor* method,
    RpcController* controller,
    google::protobuf::Message* request,
    google::protobuf::Message* response,
    const HttpRequest* http_request,
    HttpResponse* http_response,
    TextConverter* output_converter,
    Closure<void>* done)
{
    if (controller->Failed()) {
        LOG(ERROR) << controller->ErrorText();
        http_response->set_status(400);
        http_response->set_body(controller->ErrorText());
    } else {
        bool serialize_success = SerializeResponse(http_request, http_response,
                                                   output_converter, response);
        rpc_server->m_stats_registry.AddMethodCounterForUser(
            "", method->full_name(), serialize_success);
        if (serialize_success) {
            int64_t latency = GetTimeStampInMs() - begin_time;
            rpc_server->m_stats_registry.AddMethodLatencyForUser(
                "", method->full_name(), latency, begin_time);
        }
    }

    rpc_server->UnregisterRequest(controller);

    delete controller;
    delete request;
    delete response;
    done->Run();
}

void HandleServiceRequest(const RpcServerHandler* rpc_handler,
                          const HttpRequest* http_request,
                          HttpResponse* http_response,
                          Closure<void>* done)
{
    int64_t begin_time = GetTimeStampInMs();
    RpcServer* rpc_server = rpc_handler->rpc_server();

    google::protobuf::Service* service = NULL;
    const google::protobuf::MethodDescriptor* method = NULL;
    if (!GetRequestMethodInfo(rpc_handler, http_request, http_response, &service, &method)) {
        rpc_server->m_stats_registry.AddGlobalCounter(false, RPC_ERROR_METHOD_NAME);
        done->Run();
        return;
    }

    TextConverter* output_converter = NULL;
    google::protobuf::Message* request = service->GetRequestPrototype(method).New();
    if (!ParseRequestMessage(http_request, http_response, request, &output_converter)) {
        rpc_server->m_stats_registry.AddMethodCounterForUser("", method->full_name(), false,
                                                             RPC_ERROR_PARSE_REQUEST_MESSAGE);
        delete request;
        done->Run();
        return;
    }

    RpcController* controller = new RpcController;
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();
    google::protobuf::Closure* callback =
        NewClosure(TextRequestComplete, begin_time, rpc_server, method,
                   controller, request, response, http_request, http_response,
                   output_converter, done);
    if (!rpc_server->CallServiceMethod(service, method, controller, request, response, callback)) {
        delete controller;
        delete request;
        delete response;
        delete callback;

        http_response->set_status(500);
        http_response->set_body("Server is stopping");
        done->Run();
        return;
    }
}

} // namespace poppy
