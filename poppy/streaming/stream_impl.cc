// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/streaming/stream_impl.h"

#include <vector>
#include "common/base/closure.h"
#include "poppy/streaming/stream_manager.h"
#include "thirdparty/glog/logging.h"

namespace poppy {
namespace streaming {

StreamImpl::StreamImpl(int64_t stream_id,
                       const StreamOptions& options,
                       TimerManager* timer_manager) :
    m_closed(false),
    m_end_of_stream(false),
    m_stream_id(stream_id),
    m_options(options),
    m_last_packet_id(0),
    m_timer_manager(timer_manager)
{
}

StreamImpl::~StreamImpl()
{
    VLOG(1) << "In stream destructor, id: " << m_stream_id;
    ClearQueue();
}

void StreamImpl::WaitUntilClose()
{
    MutexLocker locker(m_exit_mutex);

    while (!IsUnique()) {
        m_exit_cond.Wait(&m_exit_mutex);
    }
}

void StreamImpl::ClearQueue()
{
    std::vector<CompletionCallback*> callbacks;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);

        while (!m_queue.empty()) {
            Packet* packet = m_queue.front();
            m_queue.pop_front();

            if (packet->data != NULL) {
                delete packet->data;
                packet->data = NULL;
            }

            if (packet->callback != NULL) {
                m_timer_manager->RemoveTimer(packet->timer_id);
                callbacks.push_back(packet->callback);
                packet->callback = NULL;
            }

            delete packet;
        }
    }

    for (size_t i = 0; i < callbacks.size(); ++i) {
        callbacks[i]->Run(ERROR_STREAM_CLOSED, NULL);
    }
}

void StreamImpl::Acknowledge(int64_t packet_id)
{
    while (!m_queue.empty()) {
        Packet* packet = m_queue.front();

        if (packet->id < packet_id) {
            m_queue.pop_front();

            if (packet->data != NULL) {
                delete packet->data;
                packet->data = NULL;
            }

            delete packet;
        } else {
            break;
        }
    }
}

void StreamImpl::OnTimeout(int64_t packet_id, uint64_t timer_id)
{
    CompletionCallback* callback = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);

        if (m_queue.empty()) {
            return;
        }

        size_t length = m_queue.size();

        if (packet_id < m_queue[0]->id || packet_id > m_queue[length - 1]->id) {
            // 该packet已被处理
            return;
        }

        size_t index = packet_id - m_queue[0]->id;

        if (m_queue[index]->callback != NULL) {
            m_end_of_stream = true;
            callback = m_queue[index]->callback;
            m_queue[index]->callback = NULL;

            if (m_queue[index]->data != NULL) {
                delete m_queue[index]->data;
                m_queue[index]->data = NULL;
            }
        }
    }

    if (callback) {
        callback->Run(ERROR_STREAM_TIMEOUT, NULL);
    }
}

ServerInputStream::ServerInputStream(ServerStreamManager* stream_manager,
                                     int64_t stream_id,
                                     const StreamOptions& options) :
    InputStreamImpl(stream_id, options, stream_manager->mutable_timer_manager()),
    m_stream_manager(stream_manager),
    m_buffered_request(NULL),
    m_buffered_response(NULL),
    m_buffered_done(NULL)
{
}

void ServerInputStream::ReadPacket(CompletionCallback* completion_callback)
{
    ErrorCode error_code = ERROR_SUCCESSFUL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);

        if (m_closed) {
            error_code = ERROR_STREAM_CLOSED;
        } else if (m_end_of_stream) {
            error_code = ERROR_END_OF_STREAM;
        }
    }

    if (error_code != ERROR_SUCCESSFUL) {
        completion_callback->Run(error_code, NULL);
        return;
    }

    UploadPacketRequest* request = NULL;
    UploadPacketResponse* response = NULL;
    google::protobuf::Closure* done = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);

        if (m_buffered_request != NULL) {
            request = m_buffered_request;
            response = m_buffered_response;
            done = m_buffered_done;
            m_buffered_request = NULL;
            m_buffered_response = NULL;
            m_buffered_done = NULL;
        } else {
            Packet* packet = new Packet();
            packet->type = Packet::TYPE_READ;
            packet->id = m_last_packet_id++;
            packet->callback = completion_callback;
            m_queue.push_back(packet);
            Closure<void, uint64_t>* closure = NewClosure(
                static_cast<StreamImpl*>(this), &StreamImpl::OnTimeout, packet->id);
            uint64_t timer_id =
                m_timer_manager->AddOneshotTimer(
                m_options.timeout() * m_options.retry_count(), closure);
            packet->timer_id = timer_id;
        }
    }

    if (request != NULL) {
        int64_t packet_id  = 0;
        {
            MutexLocker locker(m_mutex);
            packet_id = m_last_packet_id++;
        }
        CHECK(packet_id == request->packet_id());
        const std::string* data = request->mutable_packet_payload();
        completion_callback->Run(ERROR_SUCCESSFUL, data);
        response->set_error_code(error_code);
        done->Run();
    }
}

void ServerInputStream::OnReceive(
    google::protobuf::RpcController* controller,
    const UploadPacketRequest* request,
    UploadPacketResponse* response,
    google::protobuf::Closure* done)
{
    ErrorCode error_code = ERROR_SUCCESSFUL;
    CompletionCallback* callback = NULL;
    {
        MutexLocker locker(m_mutex);
        Acknowledge(request->packet_id());

        if (!m_queue.empty()) {
            // 只执行回调，暂不出队，需要等待ACK
            Packet* packet = m_queue.front();
            CHECK(packet->id == request->packet_id());

            if (packet->type == Packet::TYPE_EOF) {
                // 服务端流结束边界。给客户端返回EOF。
                error_code = ERROR_END_OF_STREAM;
            } else if (packet->type == Packet::TYPE_ABORT) {
                error_code = ERROR_STREAM_ABORTED;
            }

            if (packet->callback != NULL) {
                m_timer_manager->RemoveTimer(packet->timer_id);
                callback = packet->callback;
                packet->callback = NULL;
            }
        } else {
            m_buffered_request = const_cast<UploadPacketRequest*>(request);
            m_buffered_response = response;
            m_buffered_done = done;
        }
    }

    if (callback) {
        const std::string* data =
            const_cast<UploadPacketRequest*>(request)->mutable_packet_payload();
        callback->Run(ERROR_SUCCESSFUL, data);
    }

    if (m_buffered_request != NULL) {
        return;
    }

    response->set_error_code(error_code);
    done->Run();
}

void ServerInputStream::OnClientClose(
    google::protobuf::RpcController* controller,
    const CloseStreamRequest* request,
    CloseStreamResponse* response,
    google::protobuf::Closure* done)
{
    CompletionCallback* callback = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        Acknowledge(request->packet_id());
        m_end_of_stream = true; // set eof

        if (!m_queue.empty()) {
            Packet* packet = m_queue.front();
            m_queue.pop_front();
            CHECK(packet->id == request->packet_id());

            if (packet->data != NULL) {
                delete packet->data;
            }

            if (packet->callback != NULL) {
                m_timer_manager->RemoveTimer(packet->timer_id);
                callback = packet->callback;
            }

            delete packet;
        }
    }

    if (callback) {
        callback->Run(request->error_code(), NULL);
    }

    response->set_error_code(ERROR_SUCCESSFUL);
    done->Run();
}

void ServerInputStream::Close(ErrorCode error_code, Closure<void>* callback)
{
    MutexLocker locker(m_mutex);

    if (m_closed) {
        return;
    }

    m_closed = true;

    if (m_buffered_request != NULL) {
        m_buffered_response->set_error_code(error_code);
        m_buffered_done->Run();
        m_buffered_request = NULL;
        m_buffered_response = NULL;
        m_buffered_done = NULL;
    }

    Packet* packet = new Packet();
    packet->type = Packet::TYPE_EOF;

    if (error_code == ERROR_STREAM_ABORTED) {
        packet->type = Packet::TYPE_ABORT;
    }

    packet->id = m_last_packet_id++;
    CompletionCallback* close_callback = NewClosure(
            this, &ServerInputStream::CloseStream, callback);
    packet->callback = close_callback;
    m_queue.push_back(packet);
    Closure<void, uint64_t>* closure = NewClosure(
        static_cast<StreamImpl*>(this), &StreamImpl::OnTimeout, packet->id);
    uint64_t timer_id = m_timer_manager->AddOneshotTimer(
        m_options.timeout(), closure);
    packet->timer_id = timer_id;
}

void ServerInputStream::CloseStream(Closure<void>* callback,
                                    ErrorCode error_code,
                                    const std::string* data)
{
    bool end_of_stream = false;
    {
        MutexLocker locker(m_mutex);
        end_of_stream = m_end_of_stream;
    }

    if (end_of_stream) {
        m_stream_manager->RemoveInputStream(m_stream_id);

        if (callback != NULL) {
            callback->Run();
        }

        return;
    }

    // 放入一个关闭回调等待客户端先请求关闭
    MutexLocker locker(m_mutex);
    Packet* packet = new Packet();
    packet->type = Packet::TYPE_CLOSE;
    packet->id = m_last_packet_id++;
    CompletionCallback* close_complete =
        NewClosure(this, &ServerInputStream::RemoveStream, callback);
    packet->callback = close_complete;
    m_queue.push_back(packet);
    Closure<void, uint64_t>* closure = NewClosure(
        static_cast<StreamImpl*>(this),
        &StreamImpl::OnTimeout,
        packet->id);
    uint64_t timer_id = m_timer_manager->AddOneshotTimer(
        m_options.timeout(), closure);
    packet->timer_id = timer_id;
}

void ServerInputStream::RemoveStream(Closure<void>* callback,
                                     ErrorCode error_code,
                                     const std::string* data)
{
    m_stream_manager->RemoveInputStream(m_stream_id);

    if (callback != NULL) {
        callback->Run();
    }
}

ServerOutputStream::ServerOutputStream(ServerStreamManager* stream_manager,
                                       int64_t stream_id,
                                       const StreamOptions& options) :
    OutputStreamImpl(stream_id, options, stream_manager->mutable_timer_manager()),
    m_stream_manager(stream_manager),
    m_buffered_request(NULL),
    m_buffered_response(NULL),
    m_buffered_done(NULL)
{
}

void ServerOutputStream::WritePacket(const std::string* data,
                                     CompletionCallback* completion_callback)
{
    ErrorCode error_code = ERROR_SUCCESSFUL;
    {
        MutexLocker locker(m_mutex);

        if (m_closed) {
            error_code = ERROR_STREAM_CLOSED;
        }

        if (m_end_of_stream) {
            error_code = ERROR_END_OF_STREAM;
        }
    }

    if (error_code != ERROR_SUCCESSFUL) {
        delete data;
        completion_callback->Run(error_code, NULL);
        return;
    }

    DownloadPacketRequest* request = NULL;
    DownloadPacketResponse* response = NULL;
    google::protobuf::Closure* done = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);

        if (m_buffered_request != NULL) {
            request = m_buffered_request;
            response = m_buffered_response;
            done = m_buffered_done;
            m_buffered_request = NULL;
            m_buffered_response = NULL;
            m_buffered_done = NULL;
        } else {
            Packet* packet = new Packet();
            packet->type = Packet::TYPE_WRITE;
            packet->id = m_last_packet_id++;
            packet->data = const_cast<std::string*>(data);
            packet->callback = completion_callback;
            m_queue.push_back(packet);
            Closure<void, uint64_t>* closure = NewClosure(
                static_cast<StreamImpl*>(this), &StreamImpl::OnTimeout, packet->id);
            uint64_t timer_id = m_timer_manager->AddOneshotTimer(
                m_options.timeout() * m_options.retry_count(), closure);
            packet->timer_id = timer_id;
        }
    }

    if (request != NULL) {
        if (data != NULL) {
            response->set_packet_payload(*data);
        }

        {
            // for scoped lock.
            MutexLocker locker(m_mutex);
            Packet* packet = new Packet();
            packet->type = Packet::TYPE_WRITE;
            packet->id = m_last_packet_id++;
            packet->data = const_cast<std::string*>(data);
            m_queue.push_back(packet);
        }

        completion_callback->Run(ERROR_SUCCESSFUL, NULL);
        response->set_error_code(ERROR_SUCCESSFUL);
        done->Run();
    }
}

void ServerOutputStream::OnReceive(
    google::protobuf::RpcController* controller,
    const DownloadPacketRequest* request,
    DownloadPacketResponse* response,
    google::protobuf::Closure* done)
{
    ErrorCode error_code = ERROR_SUCCESSFUL;
    CompletionCallback* callback = NULL;
    {
        MutexLocker locker(m_mutex);
        Acknowledge(request->packet_id());

        if (!m_queue.empty()) {
            Packet* packet = m_queue.front();
            CHECK(packet->id == request->packet_id());

            if (packet->type == Packet::TYPE_EOF) {
                error_code = ERROR_END_OF_STREAM;
            } else if (packet->type == Packet::TYPE_ABORT) {
                error_code = ERROR_STREAM_ABORTED;
            }

            if (packet->data != NULL) {
                response->set_packet_payload(*packet->data);
                // data必须等client ack到达后才能清空
                // delete packet->data;
                // packet->data = NULL;
            }

            if (packet->callback != NULL) {
                m_timer_manager->RemoveTimer(packet->timer_id);
                callback = packet->callback;
                packet->callback = NULL;
            }
        } else {
            m_buffered_request = const_cast<DownloadPacketRequest*>(request);
            m_buffered_response = response;
            m_buffered_done = done;
        }
    }

    if (callback) {
        callback->Run(ERROR_SUCCESSFUL, NULL);
    }

    if (m_buffered_request != NULL) {
        return;
    }

    response->set_error_code(error_code);
    done->Run();
}

void ServerOutputStream::OnClientClose(
    google::protobuf::RpcController* controller,
    const CloseStreamRequest* request,
    CloseStreamResponse* response,
    google::protobuf::Closure* done)
{
    CompletionCallback* callback = NULL;
    {
        MutexLocker locker(m_mutex);
        Acknowledge(request->packet_id());
        m_end_of_stream = true;  // set eof

        if (!m_queue.empty()) {
            Packet* packet = m_queue.front();
            m_queue.pop_front();
            CHECK(packet->id == request->packet_id());

            if (packet->data != NULL) {
                delete packet->data;
                packet->data = NULL;
            }

            if (packet->callback != NULL) {
                m_timer_manager->RemoveTimer(packet->timer_id);
                callback = packet->callback;
            }

            delete packet;
        }
    }

    if (callback) {
        callback->Run(request->error_code(), NULL);
    }

    response->set_error_code(ERROR_SUCCESSFUL);
    done->Run();
}

void ServerOutputStream::Close(ErrorCode error_code, Closure<void>* callback)
{
    MutexLocker locker(m_mutex);

    if (m_closed) {
        return;
    }

    m_closed = true;

    if (m_buffered_request != NULL) {
        m_buffered_response->set_error_code(error_code);
        m_buffered_done->Run();
        m_buffered_request = NULL;
        m_buffered_response = NULL;
        m_buffered_done = NULL;
    }

    // 服务端主动关闭，延迟关闭
    Packet* packet = new Packet();
    packet->type = Packet::TYPE_EOF;

    if (error_code == ERROR_STREAM_ABORTED) {
        packet->type = Packet::TYPE_ABORT;
    }

    packet->id = m_last_packet_id++;
    CompletionCallback* close_callback =
        NewClosure(this, &ServerOutputStream::CloseStream, callback);
    packet->callback = close_callback;
    m_queue.push_back(packet);
    Closure<void, uint64_t>* closure = NewClosure(
        static_cast<StreamImpl*>(this), &StreamImpl::OnTimeout, packet->id);
    uint64_t timer_id = m_timer_manager->AddOneshotTimer(
        m_options.timeout(), closure);
    packet->timer_id = timer_id;
}

void ServerOutputStream::CloseStream(Closure<void>* callback,
                                     ErrorCode error_code,
                                     const std::string* data)
{
    bool end_of_stream = false;
    {
        MutexLocker locker(m_mutex);
        end_of_stream = m_end_of_stream;
    }

    if (end_of_stream) {
        m_stream_manager->RemoveOutputStream(m_stream_id);

        if (callback != NULL) {
            callback->Run();
        }

        return;
    }

    MutexLocker locker(m_mutex);
    Packet* packet = new Packet();
    packet->type = Packet::TYPE_CLOSE;
    packet->id = m_last_packet_id++;
    CompletionCallback* remove_callback =
        NewClosure(this, &ServerOutputStream::RemoveStream, callback);
    packet->callback = remove_callback;
    m_queue.push_back(packet);
    Closure<void, uint64_t>* closure = NewClosure(
        static_cast<StreamImpl*>(this),
        &StreamImpl::OnTimeout,
        packet->id);
    uint64_t timer_id = m_timer_manager->AddOneshotTimer(
        m_options.timeout(), closure);
    packet->timer_id = timer_id;
}

void ServerOutputStream::RemoveStream(Closure<void>* callback,
                                      ErrorCode error_code,
                                      const std::string* data)
{
    m_stream_manager->RemoveOutputStream(m_stream_id);

    if (callback != NULL) {
        callback->Run();
    }
}

ClientInputStream::ClientInputStream(ClientStreamManager* stream_manager,
                                     int64_t stream_id,
                                     const StreamOptions& options) :
    InputStreamImpl(stream_id, options, stream_manager->mutable_timer_manager()),
    m_stream_manager(stream_manager),
    m_retry_count(0)
{
}

void ClientInputStream::ReadPacket(CompletionCallback* completion_callback)
{
    ErrorCode error_code = ERROR_SUCCESSFUL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);

        if (m_closed) {
            error_code = ERROR_STREAM_CLOSED;
        } else if (m_end_of_stream) {
            error_code = ERROR_END_OF_STREAM;
        }
    }

    if (error_code != ERROR_SUCCESSFUL) {
        completion_callback->Run(error_code, NULL);
        return;
    }

    RpcController* controller = new RpcController();
    controller->SetResponseCompressType(m_options.compress_type());

    DownloadPacketRequest* request = new DownloadPacketRequest();
    DownloadPacketResponse* response = new DownloadPacketResponse();

    int64_t packet_id = 0;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        packet_id = m_last_packet_id++;
    }
    request->set_stream_id(m_stream_id);
    request->set_packet_id(packet_id);

    google::protobuf::Closure* download_packet_complete = NewClosure(this,
            &ClientInputStream::DownloadPacketComplete,
            controller,
            request,
            response,
            completion_callback);
    m_stream_manager->mutable_streaming_service_stub()->DownloadPacket(
        controller,
        request,
        response,
        download_packet_complete);
    // LOG(INFO) << "send packet: " << packet_id;
}

void ClientInputStream::DownloadPacketComplete(
    RpcController* controller,
    DownloadPacketRequest* request,
    DownloadPacketResponse* response,
    CompletionCallback* completion_callback)
{
    ErrorCode error_code = ERROR_UNKNOWN;

    if (controller->Failed()) {
        error_code = ERROR_RPC_FAILED;
        if (m_retry_count++ < m_options.retry_count()) {
            LOG(WARNING) << "Get RPC ERROR: " << controller->ErrorText()
                << ", current retry count = " << m_retry_count
                << ", max retry count = " << m_options.retry_count();
            controller->Reset();
            controller->SetResponseCompressType(m_options.compress_type());
            google::protobuf::Closure* download_packet_complete = NewClosure(this,
                    &ClientInputStream::DownloadPacketComplete,
                    controller,
                    request,
                    response,
                    completion_callback);
            m_stream_manager->mutable_streaming_service_stub()->DownloadPacket(
                    controller,
                    request,
                    response,
                    download_packet_complete);
            return;
        }
        LOG(ERROR) << "RPC ERROR: " << controller->ErrorText();
    } else {
        error_code = response->error_code();
    }
    if (error_code != ERROR_SUCCESSFUL) {
        LOG(ERROR) << "Rpc get an error response, error code: " << error_code;
    }

    if (error_code == ERROR_END_OF_STREAM || error_code == ERROR_STREAM_ABORTED) {
        MutexLocker locker(m_mutex);
        m_end_of_stream = true;
    }

    if (completion_callback != NULL) {
        // 用户回调
        completion_callback->Run(error_code, response->mutable_packet_payload());
    }

    delete controller;
    delete request;
    delete response;
}

void ClientInputStream::Close(ErrorCode error_code, Closure<void>* callback)
{
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);

        if (m_closed) {
            return;
        }

        m_closed = true;
    }

    RpcController* controller = new RpcController();
    CloseStreamRequest* request = new CloseStreamRequest();
    CloseStreamResponse* response = new CloseStreamResponse();
    int64_t packet_id = 0;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        packet_id = m_last_packet_id++;
    }
    request->set_stream_id(m_stream_id);
    request->set_packet_id(packet_id);
    request->set_error_code(error_code);

    google::protobuf::Closure* close_complete = NewClosure(this,
            &ClientInputStream::CloseComplete,
            controller,
            request,
            response,
            callback);
    m_stream_manager->mutable_streaming_service_stub()->CloseInputStream(
        controller,
        request,
        response,
        close_complete);
    // LOG(INFO) << "send packet: " << packet_id;
}

void ClientInputStream::CloseComplete(RpcController* controller,
                                      CloseStreamRequest* request,
                                      CloseStreamResponse* response,
                                      Closure<void>* callback)
{
    delete controller;
    delete request;
    delete response;
    m_stream_manager->RemoveInputStream(m_stream_id);

    if (callback != NULL) {
        callback->Run();
    }
}

ClientOutputStream::ClientOutputStream(ClientStreamManager* stream_manager,
                                       int64_t stream_id,
                                       const StreamOptions& options) :
    OutputStreamImpl(stream_id, options, stream_manager->mutable_timer_manager()),
    m_stream_manager(stream_manager),
    m_retry_count(0)
{
}

void ClientOutputStream::WritePacket(const std::string* data,
                                     CompletionCallback* completion_callback)
{
    ErrorCode error_code = ERROR_SUCCESSFUL;
    {
        MutexLocker locker(m_mutex);

        if (m_closed) {
            error_code = ERROR_STREAM_CLOSED;
        } else if (m_end_of_stream) {
            error_code = ERROR_END_OF_STREAM;
        }
    }

    if (error_code != ERROR_SUCCESSFUL) {
        completion_callback->Run(error_code, NULL);
        return;
    }

    RpcController* controller = new RpcController();
    controller->SetRequestCompressType(m_options.compress_type());

    UploadPacketRequest* request = new UploadPacketRequest();
    UploadPacketResponse* response = new UploadPacketResponse();
    controller->SetResponseCompressType(m_options.compress_type());
    int64_t packet_id = 0;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        packet_id = m_last_packet_id++;
    }
    request->set_stream_id(m_stream_id);
    request->set_packet_id(packet_id);

    if (data != NULL) {
        request->set_packet_payload(*data);
    }

    google::protobuf::Closure* upload_packet_complete = NewClosure(
                this,
                &ClientOutputStream::UploadPacketComplete,
                data,
                controller,
                request,
                response,
                completion_callback);
    m_stream_manager->mutable_streaming_service_stub()->UploadPacket(
        controller, request, response, upload_packet_complete);
    // LOG(INFO) << "send packet: " << packet_id;
}

void ClientOutputStream::UploadPacketComplete(
    const std::string* data,
    RpcController* controller,
    UploadPacketRequest* request,
    UploadPacketResponse* response,
    CompletionCallback* completion_callback)
{
    ErrorCode error_code = ERROR_UNKNOWN;

    if (controller->Failed()) {
        error_code = ERROR_RPC_FAILED;
        if (m_retry_count++ < m_options.retry_count()) {
            LOG(WARNING) << "Get RPC ERROR: " << controller->ErrorText()
                << ", current retry count = " << m_retry_count
                << ", max retry count = " << m_options.retry_count();
            controller->Reset();
            controller->SetRequestCompressType(m_options.compress_type());
            google::protobuf::Closure* upload_packet_complete = NewClosure(
                    this,
                    &ClientOutputStream::UploadPacketComplete,
                    data,
                    controller,
                    request,
                    response,
                    completion_callback);
            m_stream_manager->mutable_streaming_service_stub()->UploadPacket(
                    controller, request, response, upload_packet_complete);
            return;
        }
        LOG(WARNING) << "RPC ERROR: " << controller->ErrorText();
    } else {
        error_code = response->error_code();
    }
    if (error_code != ERROR_SUCCESSFUL) {
        LOG(ERROR) << "Rpc get an error response, error code: " << error_code;
    }

    if (error_code == ERROR_END_OF_STREAM || error_code == ERROR_STREAM_ABORTED) {
        MutexLocker locker(m_mutex);
        m_end_of_stream = true;
    }

    if (completion_callback != NULL) {
        // 用户回调
        completion_callback->Run(error_code, NULL);
    }

    delete data;
    delete controller;
    delete request;
    delete response;
}

void ClientOutputStream::Close(ErrorCode error_code, Closure<void>* callback)
{
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);

        if (m_closed) {
            return;
        }

        m_closed = true;
    }

    RpcController* controller = new RpcController();
    CloseStreamRequest* request = new CloseStreamRequest();
    CloseStreamResponse* response = new CloseStreamResponse();
    int64_t packet_id = 0;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        packet_id = m_last_packet_id++;
    }
    request->set_stream_id(m_stream_id);
    request->set_packet_id(packet_id);
    request->set_error_code(error_code);

    google::protobuf::Closure* close_complete = NewClosure(this,
            &ClientOutputStream::CloseComplete,
            controller,
            request,
            response,
            callback);
    m_stream_manager->mutable_streaming_service_stub()->CloseOutputStream(
        controller,
        request,
        response,
        close_complete);
}

void ClientOutputStream::CloseComplete(RpcController* controller,
                                       CloseStreamRequest* request,
                                       CloseStreamResponse* response,
                                       Closure<void>* callback)
{
    delete controller;
    delete request;
    delete response;
    m_stream_manager->RemoveOutputStream(m_stream_id);

    if (callback != NULL) {
        callback->Run();
    }
}

} // namespace streaming
} // namespace poppy
