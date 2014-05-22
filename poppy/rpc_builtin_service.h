// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Dongping HUANG <dphuang@tencent.com>
//         Yongsong Liu <ericliu@tencent.com>
// Created: 08/18/11
// Description:

#ifndef POPPY_RPC_BUILTIN_SERVICE_H
#define POPPY_RPC_BUILTIN_SERVICE_H
#pragma once

#include "poppy/rpc_builtin_service.pb.h"

namespace poppy {

class RpcServer;
class RpcServerHandler;

class RpcBuiltinService : public BuiltinService
{
public:
    RpcBuiltinService(RpcServer* rpc_server, RpcServerHandler* rpc_handler);

private:
    virtual void Health(google::protobuf::RpcController* controller,
                        const poppy::EmptyRequest* request,
                        HealthResponse* response,
                        google::protobuf::Closure* done);

    virtual void GetAllServiceDescriptors(
        google::protobuf::RpcController* controller,
        const poppy::EmptyRequest* request,
        AllServiceDescriptors* response,
        google::protobuf::Closure* done);

    virtual void GetMethodInputTypeDescriptors(
        google::protobuf::RpcController* controller,
        const poppy::MethodInputTypeDescriptorsRequest* request,
        MethodInputTypeDescriptorsResponse* response,
        google::protobuf::Closure* done);

    virtual ~RpcBuiltinService() {}

private:
    RpcServer* m_rpc_server;
    RpcServerHandler* m_rpc_handler;
};

} // namespace poppy

#endif // POPPY_RPC_BUILTIN_SERVICE_H
