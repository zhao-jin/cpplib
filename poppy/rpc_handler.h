// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
// Eric Liu <ericliu@tencent.com>

#ifndef POPPY_RPC_HANDLER_H
#define POPPY_RPC_HANDLER_H

#include <map>
#include <string>
#include <vector>
#include "common/base/closure.h"
#include "common/base/string/algorithm.h"
#include "common/crypto/credential/verifier.h"
#include "common/net/http/http_handler.h"
#include "poppy/rpc_builtin_service.h"
#include "poppy/rpc_controller.h"
#include "poppy/rpc_server_session_pool.h"
#include "thirdparty/protobuf/service.h"

namespace poppy {

// using namespace common;

// Parse the full name of method into service full name and method name.
// Return false if the method name is invalid.
inline bool ParseMethodFullName(const std::string& method_full_name,
                                std::string* service_full_name,
                                std::string* method_name)
{
    std::string::size_type pos = method_full_name.rfind('.');
    if (pos == std::string::npos) {
        // The method full name must have one period at least to separate
        // service name and method name.
        return false;
    }

    *service_full_name = method_full_name.substr(0, pos);
    *method_name = method_full_name.substr(pos + 1);
    return true;
}

inline bool ParseMethodFullNameFromPath(const std::string& request_path,
                                        const StringPiece& prefix,
                                        std::string* service_full_name,
                                        std::string* method_name)
{
    std::string path = request_path;
    std::string::size_type pos = path.find("?");
    if (pos != std::string::npos) {
        path = path.substr(0, pos);
    } else {
        if (StringEndsWith(path, "/")) {
            path = path.substr(0, path.size() - 1);
        }
    }
    if (!StringStartsWith(path, prefix)) {
        return false;
    }
    path = path.substr(prefix.size());
    if (StringStartsWith(path, "/")) {
        path = path.substr(1);
    }

    if (path.size() > 0) {
        return ParseMethodFullName(path, service_full_name, method_name);
    } else {
        service_full_name->clear();
        method_name->clear();
        return true;
    }
}

// The http path registered for rpc service.
extern const char kRpcHttpPath[];

// Forward declaration.
class RpcServer;

// Rpc handler. Its main task is to deserialize rpc packets
// (rpc meta and request/response) from http stream
// and dispatch to corresponding service.
class RpcServerHandler : public HttpHandler
{
public:
    explicit RpcServerHandler(RpcServer* rpc_server) :
        m_rpc_server(rpc_server) {}
    virtual ~RpcServerHandler();

    // Register a protocol buffer rpc service here. One RpcServerHandler could
    // serve multiple rpc services at the same time.
    // This is only called before server start, so no lock is used.
    void RegisterService(google::protobuf::Service* service);

    // Unregister all services bind on current handler.
    // This is only called when server destroys, so no lock is used.
    void UnregisterAllServices();

    google::protobuf::Service* FindServiceByName(const std::string& service_name) const;
    const std::map<std::string, google::protobuf::Service*>& GetAllServices() const
    {
        return m_services;
    }

    RpcServerSessionPool* mutable_session_pool()
    {
        return &m_session_pool;
    }

    RpcServer* rpc_server() const
    {
        return m_rpc_server;
    }

private:
    // Implements HttpHandler interface.
    virtual void OnClose(HttpConnection* http_connection, int error_code);
    virtual void HandleHeaders(HttpConnection* http_connection);
    virtual void HandleBodyPacket(HttpConnection* http_connection,
                                  const StringPiece& data);
    virtual int DetectBodyPacketSize(HttpConnection* http_connection,
                                     const StringPiece& data);

    void HandleRequest(RpcController* controller,
                       const RpcMeta& request_meta,
                       const StringPiece& message_data);

    bool DispatchRequest(RpcController* controller,
                         const RpcMeta& request_meta,
                         const StringPiece& message_data);

    void RequestComplete(RpcController* controller,
                         google::protobuf::Message* request,
                         google::protobuf::Message* response,
                         size_t request_memory_size);

    void VerifyUser(HttpConnection* http_connection,
                    const HttpRequest& http_request);

    void OnVerifyComplete(common::CredentialVerifier* verifier,
                          int64_t connection_id,
                          std::string credential,
                          const std::string* user_id,
                          const std::string* user_role,
                          int error_code);

    void ProcessVerifySuccess(HttpConnection* http_connection,
                              const std::string& credential,
                              const std::string& user,
                              const std::string& role);

    void ProcessVerifyRejected(HttpConnection* http_connection);

    void ProcessVerifyError(HttpConnection* http_connection);

    // Rpc services registered on the server.
    std::map<std::string, google::protobuf::Service*> m_services;

    // Rpc server handler maintains a session pool.
    // It contains all rpc connections on the server.
    RpcServerSessionPool m_session_pool;
    RpcServer* m_rpc_server;
};

class RpcConnection;

class RpcClientHandler : public HttpHandler
{
public:
    RpcClientHandler() : m_rpc_connection(NULL) {}
    virtual ~RpcClientHandler() {}

    void set_rpc_connection(RpcConnection* rpc_connection)
    {
        m_rpc_connection = rpc_connection;
    }

protected:
    // Implements HttpHandler interface.
    virtual void OnConnected(HttpConnection* connection);
    virtual void OnClose(HttpConnection* http_connection, int error_code);
    virtual void HandleHeaders(HttpConnection* http_connection);
    virtual void HandleBodyPacket(HttpConnection* http_connection,
                                  const StringPiece& data);
    virtual int DetectBodyPacketSize(HttpConnection* http_connection,
                                     const StringPiece& data);

    void SendRequest(const google::protobuf::MethodDescriptor* method,
                     RpcController* controller,
                     const google::protobuf::Message* request,
                     google::protobuf::Message* response,
                     google::protobuf::Closure* done);

protected:
    // Rpc client doesn't own a rpc, just hold a pointer.
    RpcConnection* m_rpc_connection;
};

} // namespace poppy

#endif // POPPY_RPC_HANDLER_H
