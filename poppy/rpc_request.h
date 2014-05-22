// Copyright 2011, Tencent Inc.
// Author: Xiaokangliu <hsiaokangliu@tencent.com>

#ifndef POPPY_RPC_REQUEST_H
#define POPPY_RPC_REQUEST_H

#include <string>

#include "common/base/string/string_piece.h"
#include "poppy/rpc_controller.h"

#include "glog/logging.h"
#include "protobuf/service.h"

namespace poppy {

// All data about one rpc request.
// We don't use closure as we need to re-schedule a rpc request to another
// session, and then can't bind rpc request to a session tightly.
class RpcRequest {
    // The id of the request, which is unique within the session pool.
    int64_t sequence_id;
    RpcController* controller;
    const google::protobuf::Message* request;
    const std::string request_data;
    Closure<void, const StringPiece*>* done;

    RpcRequest(int64_t sequence_id,
               RpcController* controller,
               const google::protobuf::Message* request,
               const std::string& request_data,
               Closure<void, const StringPiece*>* done) :
        sequence_id(sequence_id),
        controller(controller),
        request(request),
        request_data(request_data),
        done(done),
        is_builtin(false)
    {
        controller->set_sequence_id(sequence_id);
    }

    size_t MemoryUsage() const
    {
        return sizeof(*this) + controller->MemoryUsage() +
            (request ? request->SpaceUsed() : request_data.capacity()) +
            sizeof(*done);
    }

private:
    friend class RpcConnection;
    friend class RpcChannelImpl;
    friend class RpcRequestQueue;

    bool IsBuiltin() const { return is_builtin; }
    void SetBuiltin(bool isbuiltin) { is_builtin = isbuiltin; }

    bool is_builtin;
};

} // namespace poppy

#endif // POPPY_RPC_REQUEST_H
