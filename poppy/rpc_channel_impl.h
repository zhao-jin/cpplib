// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef POPPY_RPC_CHANNEL_IMPL_H
#define POPPY_RPC_CHANNEL_IMPL_H

#include <map>
#include <string>
#include <vector>

#include "common/base/stdext/shared_ptr.h"
#include "common/base/string/string_piece.h"
#include "common/crypto/credential/generator.h"
#include "common/net/http/http_connection.h"
#include "common/system/concurrency/event.h"
#include "common/system/concurrency/rwlock.h"

#include "poppy/rpc_builtin_service.pb.h"
#include "poppy/rpc_channel_interface.h"
#include "poppy/rpc_channel_option_info.pb.h"
#include "poppy/rpc_connection.h"
#include "poppy/rpc_controller.h"
#include "poppy/rpc_message.pb.h"
#include "poppy/rpc_meta_info.pb.h"
#include "poppy/rpc_request_queue.h"

#include "protobuf/service.h"

namespace poppy {

// This is internal event for channel, used to drive the channel, set
// by user when calling channel functions.
enum ChannelEvent {
    ChannelEvent_Disconnect = 0,
    ChannelEvent_Connect,
    ChannelEvent_Shutdowning,
    ChannelEvent_Shutdown
};

class RpcClientImpl;
class RpcWorkload;

//
// This is a internal class used by RpcHttpChannel to manage rpc
// connections. Many features is handled by this class, including
// disconnect when IDLE, reconnect on command, load balance, etc.
//
// It maintains a list of server, which are identical and stateless,
// and it manages a group of RpcConnections, which is corresponding
// to each of these servers.
//
// Once a session closes, all responses on this session are discarded,
// but all pending requests would be kept and sent again later. It
// also has a scheduling method for load balance among sessions. It
// holds a lock to protect internal table and all interface functions
// are reentrant.
//
class RpcChannelImpl : public stdext::enable_shared_from_this<RpcChannelImpl>
{
    friend class RpcClientImpl;
public:
    RpcChannelImpl(const stdext::shared_ptr<RpcClientImpl>& rpc_client_impl,
                   const std::string& name,
                   uint64_t channel_hash,
                   const std::vector<std::string>& servers,
                   common::CredentialGenerator* credential_generator,
                   const RpcChannelOptions& options);

    virtual ~RpcChannelImpl();

    void CallMethod(RpcController* controller,
                    const google::protobuf::Message* request,
                    Closure<void, const StringPiece*>* done);

    void RawCallMethod(RpcController* controller,
                       const std::string* request_data,
                       Closure<void, const StringPiece*>* done);

    bool ChangeStatus(RpcConnection* connection,
                      RpcConnection::Status prev,
                      RpcConnection::Status status);

    RpcRequest* CreateRequest(RpcController* controller,
                              const google::protobuf::Message* request,
                              const std::string* request_data,
                              Closure<void, const StringPiece*>* done);

    // Try to redispatch the undispatched requests to available servers.
    void RedispatchRequests();

    // Called when the ip addresses binded to a name changed.
    void OnAddressesChanged(const std::vector<std::string>& addresses);

    void Shutdown(bool wait_all_pending_request_finish);

    ChannelStatus GetChannelStatus();

    RpcConnection::Status GetConnectStatus();

    const std::string& GetName() const
    {
        return m_channel_name;
    }

    uint64_t GetHash() const
    {
        return m_hash;
    }

    TimerManager* GetTimerManager()
    {
        return m_timer_manager;
    }

    common::CredentialGenerator* GetCredentialGenerator()
    {
        return m_credential_generator;
    }

    const RpcChannelOptions& Options() const
    {
        return m_options;
    }

    std::string Dump() const
    {
        return "";
    }

private:
    void InternalCallMethod(RpcController* controller,
                            const google::protobuf::Message* request,
                            const std::string* request_data,
                            Closure<void, const StringPiece*>* done);

    void Close();

    // If the done is NULL, it's a synchronous call, or it's an asynchronous
    // and uses the callback inform the completion (or failure).
    // It's only called by user's thread.
    virtual void SendRequest(RpcController* controller,
                             const google::protobuf::Message* request,
                             const std::string* request_data,
                             Closure<void, const StringPiece*>* done);

    void AddConnection(const std::string& address);
    void RemoveConnection(const std::string& address);
    void AddAllConnections();
    void MakeConnectionWithDelay(int64_t delay);
    void DoMakeConnectionWithDelay(stdext::weak_ptr<RpcChannelImpl> rpc_channel_impl,
                                   uint64_t timer_id);

    RpcConnection* SelectAvailableConnection() const;

    void SyncCallback(AutoResetEvent* event,
                      Closure<void, const StringPiece*>* done,
                      const StringPiece* message_data);

    void DecrementPendingRequestCount(Closure<void, const StringPiece*>* done,
                                      const StringPiece* message_data);

    void TriggerTimerForIdle();

    // Callback for period staff
    void CheckStatus(stdext::weak_ptr<RpcChannelImpl> rpc_channel_impl,
                     uint64_t timer_id);

    bool ChangeStatusWithoutLock(RpcConnection* connection,
                                 RpcConnection::Status prev,
                                 RpcConnection::Status status);
    RpcConnection::Status GetConnectStatusWithoutLock() const;

    void SendHealthRequests();

    void AbortRequest(RpcController *controller,
                      Closure<void, const StringPiece*>* done);

    bool IsStoped() const;

    bool IsUnrecoverable() const;

    void StartTimer();

    void StopTimer();

    ChannelStatus TranslateChannelStatus(RpcConnection::Status status);

    RpcErrorCode TranslateTimeoutErrorCode(RpcConnection::Status status);

    bool HasActiveConnection() const;

    bool AllConnectionsClosed() const;

private:
    std::string m_channel_name;
    uint64_t m_hash;
    stdext::weak_ptr<RpcClientImpl> m_rpc_client_impl;
    // Server names
    std::vector<std::string> m_server_addresses;


    // RWlock to protect internal data structures.
    mutable RWLock m_connection_rwlock;

    std::vector<RpcConnection*> m_rpc_connections[RpcConnection::TotalStatusType];

    // Exit flag. if set, there is no need for reconnecting closed sessions.
    bool m_closing;

    ChannelEvent m_current_event;
    RpcConnection::Status m_connect_status;

    // Allocate for each request. Doing this in session pool instead of session
    // is to help re-schedule request among sessions.
    uint64_t m_last_sequence_id;

    TimerManager* m_timer_manager;

    RpcWorkload m_workload;
    // Hold all pending requests undispatched.
    RpcRequestQueue m_requests;

    // timer used for heart beat.
    uint64_t m_timer_id;
    // the timestamp when the channel is used latest time.
    uint64_t m_last_use_time;
    // the last timestamp when the channel check error connection
    uint64_t m_last_check_error_time;

    uint64_t m_timer_connect;

    common::CredentialGenerator* m_credential_generator;

    // Channel Options
    RpcChannelOptions m_options;

    uint64_t m_pending_request_count;
};

} // namespace poppy

#endif // POPPY_RPC_CHANNEL_IMPL_H
