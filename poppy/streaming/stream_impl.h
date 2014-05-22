// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef POPPY_STREAMING_STREAM_IMPL_H
#define POPPY_STREAMING_STREAM_IMPL_H

#include <deque>
#include <string>
#include "common/base/ref_counted.h"
#include "common/system/concurrency/condition_variable.h"
#include "common/system/concurrency/mutex.h"
#include "common/system/timer/timer_manager.h"
#include "poppy/rpc_controller.h"
#include "poppy/streaming/packet.h"

namespace poppy {
namespace streaming {

class ServerStreamManager;
class ClientStreamManager;

// Stream implementation interface
class StreamImpl : public RefCountedBase<StreamImpl>
{
public:
    StreamImpl(int64_t stream_id, const StreamOptions& options, TimerManager* tm);
    virtual ~StreamImpl();

    bool IsEOF()
    {
        MutexLocker locker(m_mutex);
        return m_end_of_stream;
    }
    void WaitUntilClose();
    void OnTimeout(int64_t packet_id, uint64_t timer_id);

    virtual void Close(ErrorCode error_code = ERROR_END_OF_STREAM,
                       Closure<void>* closure = NULL) = 0;

    int64_t stream_id() const
    {
        return m_stream_id;
    }

    bool Release()
    {
        {
            // for scoped lock.
            MutexLocker locker(m_exit_mutex);

            if (GetRefCount() == 2) {
                bool ret = RefCountedBase<StreamImpl>::Release();
                m_exit_cond.Signal();
                return ret;
            }
        }
        return RefCountedBase<StreamImpl>::Release();
    }

protected:
    void ClearQueue();
    void Acknowledge(int64_t packet_id);

    SimpleMutex m_mutex;
    SimpleMutex m_exit_mutex;
    ConditionVariable m_exit_cond;

    // 本地标识。标记为closed后不允许再对流进行操作。
    bool m_closed;
    // 远端标识。对方是否发送流结束标识。
    bool m_end_of_stream;
    // Stream唯一的身份标识
    int64_t m_stream_id;
    StreamOptions m_options;
    int64_t m_last_packet_id;
    // Stream上的发送/接收队列
    std::deque<Packet*> m_queue;
    TimerManager* m_timer_manager;
};

// InputStream interface
class InputStreamImpl : public StreamImpl
{
public:
    InputStreamImpl(int64_t stream_id,
                    const StreamOptions& options,
                    TimerManager* timer_manager) :
        StreamImpl(stream_id, options, timer_manager)
    {
    }
    virtual ~InputStreamImpl() {}
    virtual void ReadPacket(CompletionCallback* completion_callback) = 0;
};

// OutputStream interface
class OutputStreamImpl : public StreamImpl
{
public:
    OutputStreamImpl(int64_t stream_id,
                     const StreamOptions& options,
                     TimerManager* timer_manager) :
        StreamImpl(stream_id, options, timer_manager)
    {
    }
    virtual ~OutputStreamImpl() {}
    virtual void WritePacket(const std::string* data,
                             CompletionCallback* completion_callback) = 0;
};

// class ServerInputStream
class ServerInputStream : public InputStreamImpl
{
public:
    ServerInputStream(ServerStreamManager* stream_manager,
                      int64_t stream_id,
                      const StreamOptions& options);
    virtual ~ServerInputStream() {}

    // Interface implementations.
    virtual void Close(ErrorCode error_code = ERROR_END_OF_STREAM,
                       Closure<void>* callback = NULL);
    virtual void ReadPacket(CompletionCallback* completion_callback);
    // Handle the underlying rpc requests
    void OnReceive(google::protobuf::RpcController* controller,
                   const UploadPacketRequest* request,
                   UploadPacketResponse* response,
                   google::protobuf::Closure* done);
    void OnClientClose(google::protobuf::RpcController* controller,
                       const CloseStreamRequest* request,
                       CloseStreamResponse* response,
                       google::protobuf::Closure* done);
protected:
    void CloseStream(Closure<void>* callback,
                     ErrorCode error_code,
                     const std::string* data);
    void RemoveStream(Closure<void>* callback,
                      ErrorCode error_code,
                      const std::string* data);

    ServerStreamManager* m_stream_manager;
    UploadPacketRequest* m_buffered_request;
    UploadPacketResponse* m_buffered_response;
    google::protobuf::Closure* m_buffered_done;
};

// class ServerOutputStream
class ServerOutputStream : public OutputStreamImpl
{
public:
    ServerOutputStream(ServerStreamManager* stream_manager,
                       int64_t stream_id,
                       const StreamOptions& options);
    virtual ~ServerOutputStream() {}

    // Interface implementations.
    virtual void Close(ErrorCode error_code = ERROR_END_OF_STREAM,
                       Closure<void>* callback = NULL);
    virtual void WritePacket(const std::string* data,
                             CompletionCallback* completion_callback);
    // Handle the underlying rpc requests
    void OnReceive(google::protobuf::RpcController* controller,
                   const DownloadPacketRequest* request,
                   DownloadPacketResponse* response,
                   google::protobuf::Closure* done);
    void OnClientClose(google::protobuf::RpcController* controller,
                       const CloseStreamRequest* request,
                       CloseStreamResponse* response,
                       google::protobuf::Closure* done);
protected:
    void CloseStream(Closure<void>* callback,
                     ErrorCode error_code,
                     const std::string* data);
    void RemoveStream(Closure<void>* callback,
                      ErrorCode error_code,
                      const std::string* data);

    ServerStreamManager* m_stream_manager;
    DownloadPacketRequest* m_buffered_request;
    DownloadPacketResponse* m_buffered_response;
    google::protobuf::Closure* m_buffered_done;
};

// class ClientInputStream
class ClientInputStream : public InputStreamImpl
{
public:
    ClientInputStream(ClientStreamManager* stream_manager,
                      int64_t stream_id,
                      const StreamOptions& options);
    virtual ~ClientInputStream() {}

    // Interface implementations.
    virtual void Close(ErrorCode error_code = ERROR_END_OF_STREAM,
                       Closure<void>* callback = NULL);
    virtual void ReadPacket(CompletionCallback* completion_callback);
protected:
    // Read/download packet completion callback.
    void DownloadPacketComplete(RpcController* controller,
                                DownloadPacketRequest* request,
                                DownloadPacketResponse* response,
                                CompletionCallback* completion_callback);
    void CloseComplete(RpcController* controller,
                       CloseStreamRequest* request,
                       CloseStreamResponse* response,
                       Closure<void>* callback);

    ClientStreamManager* m_stream_manager;
    int32_t m_retry_count;
};

// class ClientOutputStream
class ClientOutputStream : public OutputStreamImpl
{
public:
    ClientOutputStream(ClientStreamManager* stream_manager,
                       int64_t stream_id,
                       const StreamOptions& options);
    virtual ~ClientOutputStream() {}

    // Interface implementations.
    virtual void Close(ErrorCode error_code = ERROR_END_OF_STREAM,
                       Closure<void>* callback = NULL);
    virtual void WritePacket(const std::string* data,
                             CompletionCallback* completion_callback);
protected:
    // Write/upload packet completion callback.
    void UploadPacketComplete(const std::string* data,
                              RpcController* controller,
                              UploadPacketRequest* request,
                              UploadPacketResponse* response,
                              CompletionCallback* completion_callback);
    void CloseComplete(RpcController* controller,
                       CloseStreamRequest* request,
                       CloseStreamResponse* response,
                       Closure<void>* callback);

    ClientStreamManager* m_stream_manager;
    int32_t m_retry_count;
};

} // namespace streaming
} // namespace poppy

#endif // POPPY_STREAMING_STREAM_IMPL_H
