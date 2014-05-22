// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>

#include <vector>
#include "common/base/scoped_refptr.h"
#include "poppy/rpc_controller.h"
#include "poppy/streaming/stream_impl.h"
#include "poppy/streaming/stream_manager.h"
#include "thirdparty/glog/logging.h"

#ifdef WIN32
#include <process.h>
#endif

namespace poppy {
namespace streaming {

namespace
{
static void ConvertProtobufMessage(
    google::protobuf::Message* message,
    CompletionCallback* completion_callback,
    ErrorCode error_code,
    const std::string* data)
{
    if (error_code == ERROR_SUCCESSFUL) {
        if (!message->ParsePartialFromString(*data) ||
            !message->IsInitialized()) {
            error_code = ERROR_INVALID_PB_MESSAGE;
        }
    }

    completion_callback->Run(error_code, data);
}
} // namespace

// class StreamManager
void StreamManager::WaitAllStreamClose()
{
    MutexLocker locker(m_mutex);

    while (!m_input_streams.empty() || !m_output_streams.empty()) {
        m_cond.Wait(&m_mutex);
    }
}

void StreamManager::WaitInputStreamClose(int64_t stream_id)
{
    scoped_refptr<InputStreamImpl> stream = NULL;
    {
        MutexLocker locker(m_mutex);
        InputStreamMap::iterator iter = m_input_streams.find(stream_id);

        if (iter != m_input_streams.end()) {
            stream = iter->second;
        }
    }

    if (stream) {
        stream->WaitUntilClose();
    }
}

void StreamManager::WaitOutputStreamClose(int64_t stream_id)
{
    scoped_refptr<OutputStreamImpl> stream = NULL;
    {
        MutexLocker locker(m_mutex);
        OutputStreamMap::iterator iter = m_output_streams.find(stream_id);

        if (iter != m_output_streams.end()) {
            stream = iter->second;
        }
    }

    if (stream) {
        stream->WaitUntilClose();
    }
}

void StreamManager::RemoveInputStream(int64_t stream_id)
{
    MutexLocker locker(m_mutex);
    InputStreamMap::iterator iter = m_input_streams.find(stream_id);

    if (iter != m_input_streams.end()) {
        m_input_streams.erase(iter);
        m_cond.Signal();
    }
}

void StreamManager::RemoveOutputStream(int64_t stream_id)
{
    MutexLocker locker(m_mutex);
    OutputStreamMap::iterator iter = m_output_streams.find(stream_id);

    if (iter != m_output_streams.end()) {
        m_output_streams.erase(iter);
        m_cond.Signal();
    }
}

void StreamManager::WritePacket(
    int64_t stream_id,
    const std::string* data,
    CompletionCallback* completion_callback)
{
    ErrorCode error_code = ERROR_UNKNOWN;
    scoped_refptr<OutputStreamImpl> stream = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        OutputStreamMap::const_iterator it = m_output_streams.find(stream_id);

        if (it != m_output_streams.end()) {
            stream = it->second;
        }
    }

    if (stream) {
        std::string* internal_buffer = new std::string(*data);
        stream->WritePacket(internal_buffer, completion_callback);
    } else {
        // Stream not found, an invalid stream id.
        error_code = ERROR_INVALID_STREAM;
        completion_callback->Run(error_code, NULL);
    }
}

void StreamManager::WriteMessage(
    int64_t stream_id,
    const google::protobuf::Message& message,
    CompletionCallback* completion_callback)
{
    ErrorCode error_code = ERROR_UNKNOWN;
    scoped_refptr<OutputStreamImpl> stream = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        OutputStreamMap::const_iterator it = m_output_streams.find(stream_id);

        if (it != m_output_streams.end()) {
            stream = it->second;
        }
    }

    if (stream) {
        std::string* internal_buffer = new std::string();

        if (message.IsInitialized() && message.AppendToString(internal_buffer)) {
            stream->WritePacket(internal_buffer, completion_callback);
        } else {
            delete internal_buffer;
            // Missing required fields.
            error_code = ERROR_INVALID_PB_MESSAGE;
            completion_callback->Run(error_code, NULL);
        }
    } else {
        // Stream not found, an invalid stream id.
        error_code = ERROR_INVALID_STREAM;
        completion_callback->Run(error_code, NULL);
    }
}

void StreamManager::ReadPacket(
    int64_t stream_id,
    CompletionCallback* completion_callback)
{
    ErrorCode error_code = ERROR_UNKNOWN;
    scoped_refptr<InputStreamImpl> stream = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        InputStreamMap::const_iterator it = m_input_streams.find(stream_id);

        if (it != m_input_streams.end()) {
            stream = it->second;
        }
    }

    if (stream) {
        stream->ReadPacket(completion_callback);
    } else {
        // Stream not found, an invalid stream id.
        error_code = ERROR_INVALID_STREAM;
        completion_callback->Run(error_code, NULL);
    }
}

void StreamManager::ReadMessage(
    int64_t stream_id,
    google::protobuf::Message* message,
    CompletionCallback* completion_callback)
{
    ErrorCode error_code = ERROR_UNKNOWN;
    scoped_refptr<InputStreamImpl> stream = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        InputStreamMap::const_iterator it = m_input_streams.find(stream_id);

        if (it != m_input_streams.end()) {
            stream = it->second;
        }
    }

    if (stream) {
        CompletionCallback* message_converter =
            NewClosure(
                &ConvertProtobufMessage,
                message,
                completion_callback);
        stream->ReadPacket(message_converter);
    } else {
        // Stream not found, an invalid stream id.
        error_code = ERROR_INVALID_STREAM;
        completion_callback->Run(error_code, NULL);
    }
}

// class ServerStreamManager
ServerStreamManager::~ServerStreamManager()
{
    LOG(INFO) << "In server stream manager destructor.";
    std::vector<scoped_refptr<StreamImpl> > streams;
    {
        MutexLocker locker(m_mutex);
        InputStreamMap::iterator iter1 = m_input_streams.begin();

        for (; iter1 != m_input_streams.end();) {
            scoped_refptr<InputStreamImpl> stream = iter1->second;
            InputStreamMap::iterator current_iter = iter1++;

            if (stream->IsEOF()) {
                m_input_streams.erase(current_iter);
            } else {
                streams.push_back(stream);
            }
        }

        OutputStreamMap::iterator iter2 = m_output_streams.begin();

        for (; iter2 != m_output_streams.end();) {
            scoped_refptr<OutputStreamImpl> stream = iter2->second;
            OutputStreamMap::iterator current_iter = iter2++;

            if (stream->IsEOF()) {
                m_output_streams.erase(current_iter);
            } else {
                streams.push_back(stream);
            }
        }
    }

    for (size_t i = 0; i < streams.size(); ++i) {
        streams[i]->Close();
        streams[i] = NULL;
    }

    WaitAllStreamClose();
}

int64_t ServerStreamManager::CreateInputStream(const StreamOptions& options,
        CreateStreamCallback* done)
{
    MutexLocker locker(m_mutex);
    int64_t stream_id = m_last_stream_id++;
    stream_id = (static_cast<int64_t>(getpid()) << 32) + stream_id;
    scoped_refptr<ServerInputStream> input_stream =
        new ServerInputStream(this, stream_id, options);
    CHECK(m_input_streams.insert(
            std::make_pair(stream_id, input_stream)).second)
        << "Duplicate stream id: " << stream_id;
    return stream_id;
}

int64_t ServerStreamManager::CreateOutputStream(const StreamOptions& options,
        CreateStreamCallback* done)
{
    MutexLocker locker(m_mutex);
    int64_t stream_id = m_last_stream_id++;
    stream_id = (static_cast<int64_t>(getpid()) << 32) + stream_id;
    scoped_refptr<ServerOutputStream> output_stream =
        new ServerOutputStream(this, stream_id, options);
    CHECK(m_output_streams.insert(
            std::make_pair(stream_id, output_stream)).second)
        << "Duplicate stream id: " << stream_id;
    return stream_id;
}

void ServerStreamManager::CloseIStream(int64_t stream_id,
                                       ErrorCode error_code,
                                       Closure<void>* callback)
{
    scoped_refptr<InputStreamImpl> stream = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        InputStreamMap::iterator iter = m_input_streams.find(stream_id);

        if (iter != m_input_streams.end()) {
            stream = iter->second;
        }
    }

    if (stream) {
        if (stream->IsEOF()) {
            // 流已经结束，对端已关闭
            RemoveInputStream(stream->stream_id());

            if (callback != NULL) {
                callback->Run();
            }
        } else {
            // 先通知对端，后延迟删除
            stream->Close(error_code, callback);
        }
    } else {
        if (callback) {
            callback->Run();
        }
    }
}

void ServerStreamManager::CloseOStream(int64_t stream_id,
                                       ErrorCode error_code,
                                       Closure<void>* callback)
{
    scoped_refptr<OutputStreamImpl> stream = NULL;
    {
        MutexLocker locker(m_mutex);
        OutputStreamMap::iterator iter = m_output_streams.find(stream_id);

        if (iter != m_output_streams.end()) {
            stream = iter->second;
        }
    }

    if (stream) {
        if (stream->IsEOF()) {
            // 流已经结束，对端已关闭
            RemoveOutputStream(stream->stream_id());

            if (callback != NULL) {
                callback->Run();
            }
        } else {
            // 先通知对端，后延迟删除
            stream->Close(error_code, callback);
        }
    } else {
        if (callback) {
            callback->Run();
        }
    }
}

void ServerStreamManager::OnReceiveDownloadRequest(
    google::protobuf::RpcController* controller,
    const DownloadPacketRequest* request,
    DownloadPacketResponse* response,
    google::protobuf::Closure* done)
{
    scoped_refptr<ServerOutputStream> stream = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        OutputStreamMap::iterator it = m_output_streams.find(request->stream_id());

        if (it != m_output_streams.end()) {
            stream = static_cast<ServerOutputStream*>(it->second.get());
        }
    }

    if (stream) {
        stream->OnReceive(controller, request, response, done);
    } else {
        // Stream not found, an invalid stream id.
        response->set_error_code(ERROR_INVALID_STREAM);
        done->Run();
    }
}

void ServerStreamManager::OnReceiveUploadPacket(
    google::protobuf::RpcController* controller,
    const UploadPacketRequest* request,
    UploadPacketResponse* response,
    google::protobuf::Closure* done)
{
    scoped_refptr<ServerInputStream> stream = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        InputStreamMap::iterator it = m_input_streams.find(request->stream_id());

        if (it != m_input_streams.end()) {
            stream = static_cast<ServerInputStream*>(it->second.get());
        }
    }

    if (stream) {
        stream->OnReceive(controller, request, response, done);
    } else {
        // Stream not found, an invalid stream id.
        response->set_error_code(ERROR_INVALID_STREAM);
        done->Run();
    }
}

void ServerStreamManager::OnReceiveCloseInputStreamRequest(
    google::protobuf::RpcController* controller,
    const CloseStreamRequest* request,
    CloseStreamResponse* response,
    google::protobuf::Closure* done)
{
    scoped_refptr<ServerInputStream> stream = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        InputStreamMap::iterator it = m_input_streams.find(request->stream_id());

        if (it != m_input_streams.end()) {
            stream = static_cast<ServerInputStream*>(it->second.get());
        }
    }

    if (stream) {
        stream->OnClientClose(controller, request, response, done);
    } else {
        // Stream not found, an invalid stream id.
        response->set_error_code(ERROR_INVALID_STREAM);
        done->Run();
    }
}

void ServerStreamManager::OnReceiveCloseOutputStreamRequest(
    google::protobuf::RpcController* controller,
    const CloseStreamRequest* request,
    CloseStreamResponse* response,
    google::protobuf::Closure* done)
{
    scoped_refptr<ServerOutputStream> stream = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        OutputStreamMap::iterator it = m_output_streams.find(request->stream_id());

        if (it != m_output_streams.end()) {
            stream = static_cast<ServerOutputStream*>(it->second.get());
        }
    }

    if (stream) {
        stream->OnClientClose(controller, request, response, done);
    } else {
        // Stream not found, an invalid stream id.
        response->set_error_code(ERROR_INVALID_STREAM);
        done->Run();
    }
}

// class ClientStreamManager
ClientStreamManager::~ClientStreamManager()
{
    std::vector<scoped_refptr<StreamImpl> > streams;
    {
        MutexLocker locker(m_mutex);

        for (InputStreamMap::iterator iter = m_input_streams.begin();
             iter != m_input_streams.end();
             iter++) {
            streams.push_back(iter->second);
        }

        for (OutputStreamMap::iterator iter = m_output_streams.begin();
             iter != m_output_streams.end();
             iter++) {
            streams.push_back(iter->second);
        }
    }

    for (size_t i = 0; i < streams.size(); ++i) {
        streams[i]->Close();
        streams[i] = NULL;
    }

    WaitAllStreamClose();

    if (m_own_stub) {
        delete m_streaming_service_stub;
    }
}

int64_t ClientStreamManager::CreateInputStream(const StreamOptions& options,
        CreateStreamCallback* done)
{
    RpcController* controller = new RpcController();
    CreateStreamRequest* request = new CreateStreamRequest();
    CreateStreamResponse* response = new CreateStreamResponse();
    request->mutable_options()->CopyFrom(options);

    if (done != NULL) { // asynchronous call
        Closure<void>* closure = NewClosure(this,
                                            &ClientStreamManager::CreateInputStreamComplete,
                                            controller,
                                            request,
                                            response,
                                            done);
        m_streaming_service_stub->CreateInputStream(
            controller,
            request,
            response,
            closure);
        return 0;
    } else {            // synchronous call
        m_streaming_service_stub->CreateInputStream(
            controller,
            request,
            response,
            NULL);
        ErrorCode error_code = ERROR_UNKNOWN;

        if (controller->Failed()) {
            error_code = ERROR_RPC_FAILED;
            LOG(ERROR) << "RPC ERROR: " << controller->ErrorText();
        } else {
            error_code = response->error_code();
        }

        if (error_code != ERROR_SUCCESSFUL) {
            return -1;
        }

        int64_t stream_id = response->stream_id();
        // Add to stream manager.
        {
            MutexLocker locker(m_mutex);
            scoped_refptr<ClientInputStream> input_stream = new ClientInputStream(
                this, response->stream_id(), response->options());
            CHECK(m_input_streams.insert(std::make_pair(
                        response->stream_id(), input_stream)).second)
                << "Duplicate stream id: " << response->stream_id();
        }
        delete controller;
        delete request;
        delete response;
        return stream_id;
    }
}

void ClientStreamManager::CreateInputStreamComplete(
    RpcController* controller,
    CreateStreamRequest* request,
    CreateStreamResponse* response,
    CreateStreamCallback* done)
{
    ErrorCode error_code = ERROR_UNKNOWN;

    if (controller->Failed()) {
        error_code = ERROR_RPC_FAILED;
        LOG(ERROR) << "RPC ERROR: " << controller->ErrorText();
    } else {
        error_code = response->error_code();
    }

    int64_t stream_id = -1;

    if (error_code == ERROR_SUCCESSFUL) {
        // Add to stream manager.
        MutexLocker locker(m_mutex);
        scoped_refptr<ClientInputStream> input_stream = new ClientInputStream(
            this, response->stream_id(), response->options());
        CHECK(m_input_streams.insert(std::make_pair(
                    response->stream_id(), input_stream)).second)
                << "Duplicate stream id: " << response->stream_id();
        stream_id = response->stream_id();
    }

    delete controller;
    delete request;
    delete response;
    done->Run(error_code, stream_id);
}

int64_t ClientStreamManager::CreateOutputStream(const StreamOptions& options,
        CreateStreamCallback* done)
{
    RpcController* controller = new RpcController();
    CreateStreamRequest* request = new CreateStreamRequest();
    CreateStreamResponse* response = new CreateStreamResponse();
    request->mutable_options()->CopyFrom(options);

    if (done != NULL) { // asynchronous call
        Closure<void>* closure = NewClosure(this,
                                            &ClientStreamManager::CreateOutputStreamComplete,
                                            controller,
                                            request,
                                            response,
                                            done);
        m_streaming_service_stub->CreateOutputStream(
            controller,
            request,
            response,
            closure);
        return 0;
    } else {            // synchronous call
        m_streaming_service_stub->CreateOutputStream(
            controller,
            request,
            response,
            NULL);
        ErrorCode error_code = ERROR_UNKNOWN;

        if (controller->Failed()) {
            error_code = ERROR_RPC_FAILED;
            LOG(ERROR) << "RPC ERROR: " << controller->ErrorText();
        } else {
            error_code = response->error_code();
        }

        if (error_code != ERROR_SUCCESSFUL) {
            return -1;
        }

        // Add to stream manager.
        {
            MutexLocker locker(m_mutex);
            scoped_refptr<ClientOutputStream> output_stream = new ClientOutputStream(
                this, response->stream_id(), response->options());
            CHECK(m_output_streams.insert(std::make_pair(
                        response->stream_id(), output_stream)).second)
                << "Duplicate stream id: " << response->stream_id();
        }
        int64_t stream_id = response->stream_id();
        delete controller;
        delete request;
        delete response;
        return stream_id;
    }
}

void ClientStreamManager::CreateOutputStreamComplete(
    RpcController* controller,
    CreateStreamRequest* request,
    CreateStreamResponse* response,
    CreateStreamCallback* done)
{
    ErrorCode error_code = ERROR_UNKNOWN;

    if (controller->Failed()) {
        error_code = ERROR_RPC_FAILED;
        LOG(ERROR) << "RPC ERROR: " << controller->ErrorText();
    } else {
        error_code = response->error_code();
    }

    int64_t stream_id = -1;

    if (error_code == ERROR_SUCCESSFUL) {
        // Add to stream manager.
        MutexLocker locker(m_mutex);
        scoped_refptr<ClientOutputStream> output_stream = new ClientOutputStream(
            this, response->stream_id(), response->options());
        CHECK(m_output_streams.insert(std::make_pair(
                    response->stream_id(), output_stream)).second)
            << "Duplicate stream id: " << response->stream_id();
        stream_id = response->stream_id();
    }

    delete controller;
    delete request;
    delete response;
    done->Run(error_code, stream_id);
}

void ClientStreamManager::CloseIStream(int64_t stream_id,
                                       ErrorCode error_code,
                                       Closure<void>* callback)
{
    scoped_refptr<InputStreamImpl> stream = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        InputStreamMap::iterator iter = m_input_streams.find(stream_id);

        if (iter != m_input_streams.end()) {
            stream = iter->second;
        }
    }

    if (stream) {
        stream->Close(error_code, callback);
    } else {
        if (callback) {
            callback->Run();
        }
    }
}

void ClientStreamManager::CloseOStream(int64_t stream_id,
                                       ErrorCode error_code,
                                       Closure<void>* callback)
{
    scoped_refptr<OutputStreamImpl> stream = NULL;
    {
        // for scoped lock.
        MutexLocker locker(m_mutex);
        OutputStreamMap::iterator iter = m_output_streams.find(stream_id);

        if (iter != m_output_streams.end()) {
            stream = iter->second;
        }
    }

    if (stream) {
        stream->Close(error_code, callback);
    } else {
        if (callback) {
            callback->Run();
        }
    }
}

} // namespace streaming
} // namespace poppy
