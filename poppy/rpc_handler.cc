// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
// Eric Liu <ericliu@tencent.com>

#include "poppy/rpc_handler.h"

#include <iostream>
#include "common/base/scoped_ptr.h"
#include "common/base/string/algorithm.h"
#include "common/base/string/string_number.h"
#include "common/crypto/credential/generator.h"
#include "common/encoding/percent.h"
#include "common/net/http/http_message.h"
#include "common/net/http/string_packet.h"
#include "common/net/uri/cgi_params.h"
#include "common/net/uri/uri.h"
#include "common/system/concurrency/once.h"

#include "poppy/rpc_channel_impl.h"
#include "poppy/rpc_meta_info.pb.h"
#include "poppy/rpc_server.h"

#include "thirdparty/ctemplate/template.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/protobuf/descriptor.h"
#include "thirdparty/snappy/snappy.h"

// GLOBAL_NOLINT(runtime/sizeof)

namespace poppy {

// using namespace common;

namespace {

static const char kAuthCookieName[] = "POPPY_AUTH_TICKET";
static const char kCompressTypeHeaderKey[] = "X-Poppy-Compress-Type";
static const char kPoppyTosHeaderKey[] = "X-Poppy-Tos";
static std::string s_supported_compress_types;
static DEFINE_ONCE(once_init_supported_compress_type);

// The size of the rpc packet header, which used to indicate the size of the
// whole rpc packet. Currently it's to uint32_t to indiciate the size of rpc
// meta and rpc message respectively.
static const size_t kRpcHeaderSize = 2 * sizeof(uint32_t);

static inline uint32_t BufferToHostUInt32(const char* data)
{
    return (static_cast<uint8_t>(data[0]) << 24) |
           (static_cast<uint8_t>(data[1]) << 16) |
           (static_cast<uint8_t>(data[2]) << 8)  |
           (static_cast<uint8_t>(data[3]));
}

static bool ParseCredential(const HttpRequest& http_request, std::string* credential)
{
    credential->clear();

    std::string cookie;
    if (http_request.GetHeader("Cookie", &cookie)) {
        std::vector<std::string> vec;
        SplitString(cookie, "=", &vec);
        if (vec.empty() || vec.size() > 2)
            return false;

        StringTrim(&vec[0]);
        if (vec[0] != kAuthCookieName)
            return false;

        if (vec.size() == 2) {
            *credential = vec[1];
            if (!PercentEncoding::Decode(credential))
                return false;
        }
    }

    return true;
}

static bool ParseTos(const HttpRequest& http_request, int* tos)
{
    std::string tos_value;
    if (http_request.GetHeader(kPoppyTosHeaderKey, &tos_value)) {
        return StringToNumber(tos_value, tos);
    }
    return false;
}

void GetSupportedCompressTypes()
{
    using google::protobuf::DescriptorPool;
    using google::protobuf::EnumDescriptor;
    using google::protobuf::EnumValueDescriptor;

    std::ostringstream os;
    const DescriptorPool * descriptor_pool = DescriptorPool::generated_pool();
    const EnumDescriptor* enum_desc = descriptor_pool->FindEnumTypeByName("poppy.CompressType");
    for (int i = 0; i < enum_desc->value_count(); i++) {
        const EnumValueDescriptor* enum_value_desc = enum_desc->value(i);
        if (enum_value_desc->number() == CompressTypeAuto)
            continue;
        if (!os.str().empty())
            os << ",";
        os << enum_value_desc->number();
    }
    s_supported_compress_types = os.str();
}

void SavePeerSupportedCompressTypes(HttpConnection* http_connection)
{
    http_connection->mutable_peer_supported_compress_types()->clear();
    const HttpRequest& http_request = http_connection->http_request();
    std::string x_poppy_compress_type;
    std::vector<std::string> vec;
    if (http_request.GetHeader(kCompressTypeHeaderKey, &x_poppy_compress_type)) {
        SplitString(x_poppy_compress_type, ",", &vec);
        for (size_t i = 0; i < vec.size(); i++) {
            int compress_type;
            if (StringToNumber(vec[i], &compress_type)) {
                http_connection->mutable_peer_supported_compress_types()->insert(compress_type);
            }
        }
    }
}

} // namespace

const char kRpcHttpPath[] = "/__rpc_service__";

// Class RpcServerHandler implementations.
RpcServerHandler::~RpcServerHandler()
{
    UnregisterAllServices();
}

void RpcServerHandler::UnregisterAllServices()
{
    std::map<std::string, google::protobuf::Service*>::iterator iter;

    for (iter = m_services.begin(); iter != m_services.end(); ++iter) {
        delete iter->second;
    }

    m_services.clear();
}

void RpcServerHandler::RegisterService(google::protobuf::Service* service)
{
    const std::string& service_name = service->GetDescriptor()->full_name();
    CHECK(m_services.insert(make_pair(service_name, service)).second)
            << "Duplicated service on name: " << service_name;
}

google::protobuf::Service*
RpcServerHandler::FindServiceByName(const std::string& service_name) const
{
    std::map<std::string, google::protobuf::Service*>::const_iterator it =
        m_services.find(service_name);
    if (it == m_services.end()) {
        return NULL;
    }

    return it->second;
}

void RpcServerHandler::OnClose(HttpConnection* http_connection,
                               int error_code)
{
    VLOG(3) << "Close Session: " << http_connection->GetRemoteAddress().ToString();
    m_session_pool.OnClose(http_connection->GetConnectionId(), error_code);
}

void RpcServerHandler::VerifyUser(HttpConnection* http_connection,
                                  const HttpRequest& http_request)
{
    std::string credential;
    if (!ParseCredential(http_request, &credential)) {
        ProcessVerifyRejected(http_connection);
        return;
    }

    VLOG(3) << "New Session: " << http_connection->GetRemoteAddress().ToString();
    m_session_pool.OnNewSession(http_connection);

    common::CredentialVerifier* verifier = m_rpc_server->GetCredentialVerifier();
    common::CredentialVerifier::VerifyFinishedCallback* callback =
        NewClosure(this, &RpcServerHandler::OnVerifyComplete,
                   verifier, http_connection->GetConnectionId(), credential);
    verifier->Verify(credential,
                     http_connection->GetRemoteAddress().ToString(),
                     callback);
}

void RpcServerHandler::OnVerifyComplete(
    common::CredentialVerifier* verifier,
    int64_t connection_id,
    std::string credential,
    const std::string* user_id,
    const std::string* user_role,
    int error_code)
{
    HttpConnection* http_connection = m_session_pool.GetHttpConnection(connection_id);
    if (http_connection == NULL) {
        return;
    }

    if (error_code == 0) {
        ProcessVerifySuccess(http_connection, credential, *user_id, *user_role);
    } else {
        if (verifier->IsRejected(error_code))
            ProcessVerifyRejected(http_connection);
        else
            ProcessVerifyError(http_connection);
    }
}

void RpcServerHandler::ProcessVerifySuccess(HttpConnection* http_connection,
                                            const std::string& credential,
                                            const std::string& user,
                                            const std::string& role)
{
    DCHECK_NOTNULL(dynamic_cast<HttpServerConnection*>(http_connection)); // NOLINT(runtime/rtti)
    HttpServerConnection* http_server_connection =
        static_cast<HttpServerConnection*>(http_connection);
    http_server_connection->set_credential(credential);
    http_server_connection->set_user(user);
    http_server_connection->set_role(role);

    once_init_supported_compress_type.Init(&GetSupportedCompressTypes);

    std::string response = "HTTP/1.1 200 OK\r\n";
    response.append(kCompressTypeHeaderKey);
    response.append(": ");
    response.append(s_supported_compress_types);
    response.append("\r\n\r\n");
    if (!http_connection->SendPacket(new StringPacket(&response))) {
        http_connection->Close();
        return;
    }

    SavePeerSupportedCompressTypes(http_connection);
}

void RpcServerHandler::ProcessVerifyRejected(HttpConnection* http_connection)
{
    std::string response = "HTTP/1.1 403 Forbidden\r\n\r\n";
    http_connection->SendPacket(new StringPacket(&response));
    http_connection->Close();
}

void RpcServerHandler::ProcessVerifyError(HttpConnection* http_connection)
{
    std::string response = "HTTP/1.1 401 Unauthorized\r\n\r\n";
    http_connection->SendPacket(new StringPacket(&response));
    http_connection->Close();
}

void RpcServerHandler::HandleHeaders(HttpConnection* http_connection)
{
    const HttpRequest& http_request = http_connection->http_request();
    if (http_request.method() != HttpRequest::METHOD_POST) {
#if defined(LIST_TODO)
#pragma message("\nTODO(ericliu) Http协议组包逻辑抽象并统一错误处理\n" \
                "组包方式应该如下：\n" \
                "HttpResponse response;\n" \
                "response.set_status(400);\n" \
                "response.set_body(\"Invalid HTTP method, only POST is acceptable\");")
#endif
        // Only expect post method.
        std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n"
                               "Invalid HTTP method, only POST is acceptable";
        http_connection->SendPacket(new StringPacket(&response));
        http_connection->Close();
        return;
    }

    int tos;
    if (ParseTos(http_request, &tos)) {
        http_connection->SetTos(tos);
    }

    VerifyUser(http_connection, http_request);
}

int RpcServerHandler::DetectBodyPacketSize(HttpConnection* http_connection,
        const StringPiece& data)
{
    if (data.size() < kRpcHeaderSize) {
        return 0;
    }

    const char* buffer = data.data();

    // 4bit meta length
    uint32_t meta_length = BufferToHostUInt32(buffer);

    buffer += sizeof(uint32_t);

    // 4bit message length
    uint32_t message_length = BufferToHostUInt32(buffer);

    // total_length = 4 * 2 + meta_length + message_length
    return kRpcHeaderSize + meta_length + message_length;
}

void RpcServerHandler::HandleBodyPacket(HttpConnection* http_connection,
                                        const StringPiece& data)
{
    const char* buffer = data.data();
    uint32_t meta_length = BufferToHostUInt32(buffer);
    buffer += sizeof(uint32_t);
    uint32_t message_length = BufferToHostUInt32(buffer);
    buffer += sizeof(uint32_t);

    CHECK_EQ(data.size(), kRpcHeaderSize + meta_length + message_length);

    RpcMeta request_meta;
    if (!request_meta.ParseFromArray(buffer, meta_length)) {
        // Meta is invalid, unexpected, close the connection.
#ifdef NDEBUG
        LOG(WARNING) << "Failed to parse the rpc meta, it is missing some required fields";
#else
        LOG(WARNING) << "Failed to parse the rpc meta, it is missing required fields: "
            << request_meta.InitializationErrorString();
#endif
        std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
        http_connection->SendPacket(new StringPacket(&response));
        http_connection->Close();
        m_rpc_server->m_stats_registry.AddGlobalCounter(false, RPC_ERROR_PARSE_REQUEST_MESSAGE);
        return;
    }

    if (request_meta.sequence_id() < 0) {
        // Sequence id is invalid, unexpected, close the connection.
        LOG(WARNING) << "Invalid rpc sequence id:";
        std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
        http_connection->SendPacket(new StringPacket(&response));
        http_connection->Close();
        m_rpc_server->m_stats_registry.AddGlobalCounter(false, RPC_ERROR_PARSE_REQUEST_MESSAGE);
        return;
    }

    VLOG(4) << "Receive request from connection: "
            << http_connection->GetConnectionId() << ", sequence_id :"
            << request_meta.sequence_id();

    RpcController* controller = new RpcController();
    controller->set_time(GetTimeStampInMs());
    controller->set_connection_id(http_connection->GetConnectionId());
    controller->set_sequence_id(request_meta.sequence_id());
    controller->set_remote_address(http_connection->GetRemoteAddress());
    controller->SetTimeout(request_meta.timeout());

    DCHECK_NOTNULL(dynamic_cast<HttpServerConnection*>(http_connection)); // NOLINT(runtime/rtti)
    HttpServerConnection* http_server_connection =
        static_cast<HttpServerConnection*>(http_connection);
    controller->set_credential(http_server_connection->credential());
    controller->set_user(http_server_connection->user());
    controller->set_role(http_server_connection->role());

    buffer += meta_length;
    StringPiece message_data(buffer, message_length);
    std::string uncompressed_message;

    if (request_meta.has_compress_type()) {
        CompressType request_compress_type = request_meta.compress_type();

        switch (request_compress_type) {
        case CompressTypeNone:
            break;
        case CompressTypeSnappy:

            if (snappy::Uncompress(message_data.data(), message_data.size(),
                                   &uncompressed_message)) {
                message_data.set(uncompressed_message.data(),
                                 uncompressed_message.size());
            } else {
                LOG(ERROR) << "Fail to uncompress the message data. "
                           << "sequence id: " << request_meta.sequence_id();
                controller->SetFailed(RPC_ERROR_UNCOMPRESS_MESSAGE);
            }

            break;
        default:
            LOG(ERROR) << "Unknown compress type. sequence id: "
                       << "sequence id: " << request_meta.sequence_id();
            controller->SetFailed(RPC_ERROR_COMPRESS_TYPE);
        }
    }

    HandleRequest(controller, request_meta, message_data);
}

void RpcServerHandler::HandleRequest(RpcController* controller,
                                     const RpcMeta& request_meta,
                                     const StringPiece& message_data)
{
    if (controller->Failed() ||
        !DispatchRequest(controller, request_meta, message_data)) {
        // Failed to dispatch rpc request, send an error message to client and
        // release the controller.
        m_rpc_server->m_stats_registry.AddGlobalCounter(false, controller->ErrorCode());
        // Send error response.
        m_session_pool.SendResponse(controller, NULL);
        delete controller;
    }
}

bool RpcServerHandler::DispatchRequest(RpcController* controller,
                                       const RpcMeta& request_meta,
                                       const StringPiece& message_data)
{
    if (!request_meta.has_method()) {
        LOG(WARNING) << "Request has no method name, sequence_id: " << request_meta.sequence_id();
        controller->SetFailed(RPC_ERROR_NOT_SPECIFY_METHOD_NAME);
        return false;
    }

    const std::string& method_full_name = request_meta.method();
    std::string service_name;
    std::string method_name;
    if (!ParseMethodFullName(method_full_name, &service_name, &method_name)) {
        LOG(WARNING) << "Method full name " << method_full_name << " is invalid";
        controller->SetFailed(RPC_ERROR_METHOD_NAME, method_full_name);
        return false;
    }

    std::map<std::string, google::protobuf::Service*>::const_iterator it =
        m_services.find(service_name);
    if (it == m_services.end()) {
        LOG(WARNING) << "Service " << service_name
            << " not found, method full name is " << method_full_name;
        controller->SetFailed(RPC_ERROR_FOUND_SERVICE, method_full_name);
        return false;
    }

    google::protobuf::Service* service = it->second;
    const google::protobuf::MethodDescriptor* method =
        service->GetDescriptor()->FindMethodByName(method_name);
    if (method == NULL) {
        LOG(WARNING) << "Method " << method_name << " not found, method full name is "
            << method_full_name;
        controller->SetFailed(RPC_ERROR_FOUND_METHOD, method_full_name);
        return false;
    }

    if (request_meta.has_expected_response_compress_type() &&
        request_meta.expected_response_compress_type() != CompressTypeAuto) {
        controller->SetResponseCompressType(
            request_meta.expected_response_compress_type());
    } else {
        if (method->options().HasExtension(response_compress_type)) {
            controller->SetResponseCompressType(
                method->options().GetExtension(response_compress_type));
        } else {
            controller->SetResponseCompressType(CompressTypeNone);
        }
    }

    google::protobuf::Message* request =
        service->GetRequestPrototype(method).New();
    if (!request->ParseFromArray(message_data.data(), message_data.size())) {
#ifdef NDEBUG
        LOG(WARNING) << "Failed to parse the request message, it is missing some required fields";
#else
        LOG(WARNING) << "Failed to parse the request message, it is missing required fields: "
            << request->InitializationErrorString();
#endif
        controller->SetFailed(RPC_ERROR_PARSE_REQUEST_MESSAGE);
        delete request;
        return false;
    }

    google::protobuf::Message* response =
        service->GetResponsePrototype(method).New();
    controller->set_method_full_name(method_full_name);
    size_t request_memory_size = controller->MemoryUsage() + request->SpaceUsed();
    m_rpc_server->IncreaseRequestMemorySize(request_memory_size);

    google::protobuf::Closure* callback = NewClosure(this,
                                          &RpcServerHandler::RequestComplete,
                                          controller,
                                          request,
                                          response,
                                          request_memory_size);

    // Call the method. Response will be sent in the callback function
    // and after then new memory will be deleted.
    if (!m_rpc_server->CallServiceMethod(service, method, controller, request,
                                         response, callback)) {
        m_rpc_server->DecreaseRequestMemorySize(request_memory_size);
        controller->SetFailed(RPC_ERROR_SERVER_SHUTDOWN);
        delete callback;
        delete response;
        delete request;
        return false;
    }

    return true;
}

void RpcServerHandler::RequestComplete(
    RpcController* controller,
    google::protobuf::Message* request,
    google::protobuf::Message* response,
    size_t request_memory_size)
{
    CHECK_LE(response->ByteSize(), static_cast<int>(kMaxResponseSize))
        << controller->method_full_name() << ": Response too large";

    m_session_pool.SendResponse(controller, response);
    if (!controller->Failed()) {
        m_rpc_server->m_stats_registry.AddMethodCounterForUser(controller->User(),
                controller->method_full_name(), true);
    } else {
        m_rpc_server->m_stats_registry.AddMethodCounterForUser(controller->User(),
                controller->method_full_name(), false, controller->ErrorCode());
    }
    m_rpc_server->m_stats_registry.AddMethodLatencyForUser(controller->User(),
            controller->method_full_name(),
            (GetTimeStampInMs() - controller->Time()), controller->Time() / 1000);

    m_rpc_server->DecreaseRequestMemorySize(request_memory_size);
    m_rpc_server->UnregisterRequest(controller);
    delete controller;
    delete request;
    delete response;
}

// Class RpcClientHandler implementations
void RpcClientHandler::OnConnected(HttpConnection* http_connection)
{
    VLOG(2) << "Connected to " << http_connection->GetRemoteAddress().ToString();

    common::CredentialGenerator* credential_generator = m_rpc_connection->GetCredentialGenerator();
    std::string credential;
    if (!credential_generator->Generate(&credential)) {
        LOG(WARNING) << "Can't generate credential";
        http_connection->Close();
    }

    once_init_supported_compress_type.Init(&GetSupportedCompressTypes);

    std::string request = "POST ";
    request.append(kRpcHttpPath);
    request.append(" HTTP/1.1\r\n");
    request.append("Cookie: ");
    request.append(kAuthCookieName);
    request.append("=");
    PercentEncoding::EncodeUriComponentAppend(credential, &request);
    request.append("\r\n");
    request.append(kCompressTypeHeaderKey);
    request.append(": ");
    request.append(s_supported_compress_types);
    request.append("\r\n");
    request.append(kPoppyTosHeaderKey);
    request.append(": ");
    request.append(NumberToString(
            m_rpc_connection->GetRpcChannelImpl()->Options().tos()
            ));
    request.append("\r\n\r\n");

    if (!http_connection->SendPacket(new StringPacket(&request))) {
        // Failed to send packet, close the connection.
        LOG(WARNING) << "Failed to send login after connected to "
                     << http_connection->GetRemoteAddress().ToString();
        http_connection->Close();
    }
}

void RpcClientHandler::OnClose(HttpConnection* http_connection,
                               int error_code)
{
    VLOG(3) << "Close Session: " << http_connection->GetRemoteAddress().ToString();
    m_rpc_connection->OnClose(http_connection, error_code);
}

void RpcClientHandler::HandleHeaders(HttpConnection* http_connection)
{
    HttpResponse* http_response = http_connection->mutable_http_response();

    // Check if connected successfully.
    if (http_response->status() != 200) {
        LOG(WARNING) << "Login Failed, HTTP Status: " << http_response->status()
                     << ", server address: " << http_connection->GetRemoteAddress().ToString();
        http_connection->Close();
        return;
    }

    SavePeerSupportedCompressTypes(http_connection);

    VLOG(3) << "New Session: " << http_connection->GetRemoteAddress().ToString();
    m_rpc_connection->OnNewSession(http_connection);
}

int RpcClientHandler::DetectBodyPacketSize(HttpConnection* http_connection,
        const StringPiece& data)
{
    if (data.size() < kRpcHeaderSize) {
        return 0;
    }

    const char* buffer = data.data();
    // 4bit meta length
    uint32_t meta_length = BufferToHostUInt32(buffer);
    buffer += sizeof(uint32_t);
    // 4bit message length
    uint32_t message_length = BufferToHostUInt32(buffer);
    // total_length = 4 * 2 + meta_length + message_length
    return kRpcHeaderSize + meta_length + message_length;
}

void RpcClientHandler::HandleBodyPacket(HttpConnection* http_connection,
                                        const StringPiece& data)
{
    const char* buffer = data.data();
    uint32_t meta_length = BufferToHostUInt32(buffer);
    buffer += sizeof(uint32_t);
    uint32_t message_length = BufferToHostUInt32(buffer);
    buffer += sizeof(uint32_t);

    CHECK_EQ(data.size(), kRpcHeaderSize + meta_length + message_length);

    StringPiece meta(buffer, meta_length);
    buffer += meta_length;
    StringPiece message_data(buffer, message_length);
    m_rpc_connection->HandleResponse(meta, message_data);
}

} // namespace poppy
