// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/streaming/streaming_service.h"
#include "poppy/streaming/stream_manager.h"
#include "thirdparty/glog/logging.h"

namespace poppy {
namespace streaming {

// class StreamingServiceImpl
StreamingServiceImpl::StreamingServiceImpl()
    : m_stream_manager(new ServerStreamManager())
{
}

StreamingServiceImpl::~StreamingServiceImpl()
{
    LOG(INFO) << "in streaming service dtor.";
    delete m_stream_manager;
}

void StreamingServiceImpl::CreateInputStream(
    google::protobuf::RpcController* controller,
    const CreateStreamRequest* request,
    CreateStreamResponse* response,
    google::protobuf::Closure* done)
{
    // 客户端创建InputStream，服务端对应OutputStream
    int64_t stream_id =
        m_stream_manager->CreateOutputStream(request->options());
    response->mutable_options()->CopyFrom(request->options());
    response->set_error_code(ERROR_SUCCESSFUL);
    response->set_stream_id(stream_id);
    done->Run();
}

void StreamingServiceImpl::CreateOutputStream(
    google::protobuf::RpcController* controller,
    const CreateStreamRequest* request,
    CreateStreamResponse* response,
    google::protobuf::Closure* done)
{
    // 客户端创建OutputStream，服务端对应InputStream
    int64_t stream_id =
        m_stream_manager->CreateInputStream(request->options());
    response->mutable_options()->CopyFrom(request->options());
    response->set_error_code(ERROR_SUCCESSFUL);
    response->set_stream_id(stream_id);
    done->Run();
}

void StreamingServiceImpl::CloseInputStream(
    google::protobuf::RpcController* controller,
    const CloseStreamRequest* request,
    CloseStreamResponse* response,
    google::protobuf::Closure* done)
{
    // 客户端关闭InputStream，服务端关闭OutputStream
    m_stream_manager->OnReceiveCloseOutputStreamRequest(
        controller, request, response, done);
}

void StreamingServiceImpl::CloseOutputStream(
    google::protobuf::RpcController* controller,
    const CloseStreamRequest* request,
    CloseStreamResponse* response,
    google::protobuf::Closure* done)
{
    // 客户端关闭OutputStream，服务端对应InputStream
    m_stream_manager->OnReceiveCloseInputStreamRequest(
        controller, request, response, done);
}

void StreamingServiceImpl::DownloadPacket(
    google::protobuf::RpcController* controller,
    const DownloadPacketRequest* request,
    DownloadPacketResponse* response,
    google::protobuf::Closure* done)
{
    m_stream_manager->OnReceiveDownloadRequest(
        controller, request, response, done);
}

void StreamingServiceImpl::UploadPacket(
    google::protobuf::RpcController* controller,
    const UploadPacketRequest* request,
    UploadPacketResponse* response,
    google::protobuf::Closure* done)
{
    m_stream_manager->OnReceiveUploadPacket(
        controller, request, response, done);
}

} // namespace streaming
} // namespace poppy
