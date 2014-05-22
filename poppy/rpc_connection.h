// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: DongPing HUANG <dphuang@tencent.com>
// Created: 11/14/11
// Description:

#ifndef POPPY_RPC_CONNECTION_H
#define POPPY_RPC_CONNECTION_H
#pragma once

#include <string>

#include "common/base/stdext/shared_ptr.h"
#include "common/net/http/http_server.h"
#include "common/system/concurrency/mutex.h"

#include "poppy/rpc_builtin_service.pb.h"
#include "poppy/rpc_controller.h"
#include "poppy/rpc_meta_info.pb.h"
#include "poppy/rpc_request_queue.h"

namespace common {
    class CredentialGenerator;
}

namespace poppy {

class RpcRequest;
class RpcChannelImpl;
class RpcClientImpl;
class RpcWorkload;

//
// RpcConnection is a connection to a ip:port address. The tcp connection
// to the server is on demand. The tcp connection will be disconnected on
// idle to save system resouece, if there isn't traffic on it in a given
// time span, the connection is IDLE, the time span is managed by channel
// impl in its channel options.
// RpcConnection is responsible for serialze requests and send them to rpc
// server, deserialize responses and trigger the completion closure.
//
class RpcConnection
{
public:
    // Status of the connection
    // Only connection in Healthy status can send requests; Connected ones
    // can send heart beat request to server, and switch to Healthy if the
    // server replyes to say it's healthy.
    enum Status {
        Healthy = 0,
        Connected,
        Connecting,
        Disconnecting,
        Disconnected,
        ConnectError,
        Unrecoverable,  // this is a split for those un-recoverable
        NoAuth,
        Shutdown,
        TotalStatusType
    };

public:
    RpcConnection(RpcClientImpl* rpc_client_impl,
                  const stdext::shared_ptr<RpcChannelImpl>& channel_impl,
                  const std::string& address);

    virtual ~RpcConnection();

    bool Connect();

    bool Disconnect();

    // Called when a new http connection has been verified as a valid rpc
    // session.
    virtual void OnNewSession(HttpConnection* http_connection);
    // Called when a http connection is closed.
    virtual void OnClose(HttpConnection* http_connection, int error_code);

    virtual bool SendRequest(RpcRequest* rpc_request, RpcErrorCode* error_code = NULL);

    virtual void HandleResponse(StringPiece meta,
                                StringPiece data);

    void SendHealthRequest();

    RpcConnection::Status GetConnectStatus() const;

    const std::string& GetAddress() const;

    uint64_t GetPendingRequestsCount() const;

    bool IsHealthy() const { return m_healthy; }

    common::CredentialGenerator* GetCredentialGenerator();

    RpcWorkload* GetWorkload();

    RpcChannelImpl* GetRpcChannelImpl() const;

private:
    void ChangeStatus(Status status);

    bool SendMessage(const RpcMeta& rpc_meta,
                     const StringPiece& message_data,
                     RpcErrorCode* error_code = NULL);

    void HealthCallback(poppy::RpcController* controller,
                        poppy::EmptyRequest* request,
                        poppy::HealthResponse* response,
                        const StringPiece* message_data);

    void HandleHealthChange(bool healthy);

private:
    RpcClientImpl* m_rpc_client_impl;
    stdext::weak_ptr<RpcChannelImpl> m_channel_impl;
    HttpConnection *m_http_connection;
    std::string m_address;
    TimerManager* m_timer_manager;

    // TODO(phongchen): We need RecursiveMutex because following fucntion path acquires mutex twice:
    // OnClose() => rpc_request::CancelAll() => HealthCallback() as done() => HandleHealthChange()
    // It acquires mutex in both OnClose() and HealthCallback()
    // Shortterm solution: use RecursiveMutex instead of Mutex, and here it is.
    // Longterm solution: move callback in CancelAll() out of mutex's scope.
    //
    // The mutex to protect state
    mutable RecursiveMutex m_mutex;
    Status m_status;
    bool m_healthy;

    // pending requests
    RpcRequestQueue m_requests;
    uint64_t m_pending_builtin_count;
};

} // namespace poppy

#endif // POPPY_RPC_CONNECTION_H
