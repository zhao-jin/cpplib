// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
//
// Implement the rpc streaming service.

#ifndef POPPY_STREAMING_STREAMING_SERVICE_H
#define POPPY_STREAMING_STREAMING_SERVICE_H

#include "poppy/streaming/streaming_service_info.pb.h"

namespace poppy
{
namespace streaming
{

class ServerStreamManager;

class StreamingServiceImpl : public StreamingService
{
public:
    StreamingServiceImpl();
    virtual ~StreamingServiceImpl();

    ServerStreamManager* mutable_stream_manager()
    {
        return m_stream_manager;
    }

private:
    virtual void CreateInputStream(
        google::protobuf::RpcController* controller,
        const CreateStreamRequest* request,
        CreateStreamResponse* response,
        google::protobuf::Closure* done);

    virtual void CreateOutputStream(
        google::protobuf::RpcController* controller,
        const CreateStreamRequest* request,
        CreateStreamResponse* response,
        google::protobuf::Closure* done);

    virtual void CloseInputStream(
        google::protobuf::RpcController* controller,
        const CloseStreamRequest* request,
        CloseStreamResponse* response,
        google::protobuf::Closure* done);

    virtual void CloseOutputStream(
        google::protobuf::RpcController* controller,
        const CloseStreamRequest* request,
        CloseStreamResponse* response,
        google::protobuf::Closure* done);

    virtual void DownloadPacket(
        google::protobuf::RpcController* controller,
        const DownloadPacketRequest* request,
        DownloadPacketResponse* response,
        google::protobuf::Closure* done);

    virtual void UploadPacket(
        google::protobuf::RpcController* controller,
        const UploadPacketRequest* request,
        UploadPacketResponse* response,
        google::protobuf::Closure* done);
protected:
    ServerStreamManager* m_stream_manager;
};

} // namespace streaming
} // namespace poppy

#endif // POPPY_STREAMING_STREAMING_SERVICE_H
