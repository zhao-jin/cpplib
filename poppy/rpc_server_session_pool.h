// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef POPPY_RPC_SERVER_SESSION_POOL_H
#define POPPY_RPC_SERVER_SESSION_POOL_H

#include <map>
#include "common/base/string/string_piece.h"
#include "common/net/http/http_connection.h"
#include "common/system/concurrency/rwlock.h"
#include "poppy/rpc_controller.h"
#include "poppy/rpc_meta_info.pb.h"
#include "thirdparty/protobuf/service.h"

namespace poppy {

// using namespace common;

// A collection of rpc session. It's a "passive" session pool and only used at
// server side, which means, once a session closes, we do NOT try to reconnect
// it and all pending responses on this session are discarded.
// It holds a lock to protect internal table and all interface functions are
// reentrant.
class RpcServerSessionPool
{
public:
    RpcServerSessionPool() {}
    virtual ~RpcServerSessionPool();

    // Called when a new http connection has been verified as a valid rpc
    // session.
    virtual void OnNewSession(HttpConnection* http_connection);
    // Called when a http connection is closed.
    virtual void OnClose(int64_t connection_id, int error_code);

    virtual void SendResponse(
        const RpcController* controller,
        const google::protobuf::Message* response);

    HttpConnection* GetHttpConnection(int64_t connection_id);

protected:
    // This function is possbily called by netframe thread and user's thread,
    // be careful about the reentrance.
    static void SendMessage(HttpConnection* http_connection,
                            const RpcMeta& response_meta,
                            const StringPiece& message_data);

    // Set the session pool as closing. Providing such a function instead of
    // doing in destructor as we need to wait for all http connections are
    // really closed (OnClose is called) before delete the session pool.
    virtual void CloseAllSessions();

    // The rwlock to protect state of connections.
    RWLock m_connection_rwlock;
    typedef std::map<int64_t, HttpConnection*> ConnectionMap;
    // Now a rpc session is simply a http connection. So the session pool does
    // NOT own the rpc sessions, the netframe does.
    ConnectionMap m_rpc_sessions;
};

} // namespace poppy

#endif // POPPY_RPC_SERVER_SESSION_POOL_H
