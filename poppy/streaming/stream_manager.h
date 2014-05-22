// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef POPPY_STREAMING_STREAM_MANAGER_H
#define POPPY_STREAMING_STREAM_MANAGER_H

#include <map>
#include <string>
#include "common/base/scoped_refptr.h"
#include "common/system/concurrency/condition_variable.h"
#include "common/system/concurrency/mutex.h"
#include "poppy/streaming/stream_impl.h"
#include "poppy/streaming/streaming_service_info.pb.h"
#include "thirdparty/glog/logging.h"

namespace poppy {
namespace streaming {

// The StreamManager provides all functions about a stream. The "stream_id"
// is a descriptor about a stream. We don't provide RpcStreamWriter and
// RpcStreamReader class as in the asynchronous model, it's more convenient that
// the framework manages all contexts/states than the user holding a pointer to
// RpcStreamWriter/RpcStreamReader.
//
// Basically it implements a "proactor" asynchronous model:
// http://www.cs.wustl.edu/~schmidt/PDF/proactor.pdf
//
// TODO(hsiaokangliu): provides wrapper functions for synchronous operations.

class StreamManager
{
public:
    typedef ::Closure<void, ErrorCode, int64_t> CreateStreamCallback;

    StreamManager() {}
    virtual ~StreamManager() {}

    // The write/read functions.

    // The following two functions will both allocate an internal buffer to
    // store the data copied from the write input data.
    virtual void WritePacket(int64_t stream_id,
                             const std::string* data,
                             CompletionCallback* completion_callback);

    virtual void WriteMessage(int64_t stream_id,
                              const google::protobuf::Message& message,
                              CompletionCallback* completion_callback);

    // Read a packet from stream.
    //
    // The received packet would be stored in an internal buffer and passed to
    // the callback. The internal buffer would be INVALID once the callback
    // returns and so it's caller's responsibility to store it somewhere else if
    // necessary.
    virtual void ReadPacket(int64_t stream_id,
                            CompletionCallback* completion_callback);

    // A wrapper function for protocol buffer message.
    //
    // NOTE: the caller must guarantee the "message" is valid until the
    // "completion_callback" is called. The major reason it's more convenient
    // that the caller allocates the "message" as it knows its type.
    virtual void ReadMessage(int64_t stream_id,
                             google::protobuf::Message* message,
                             CompletionCallback* completion_callback);

    // Behavior on server and client side is different
    // when creating and closing a stream
    virtual int64_t CreateInputStream(const StreamOptions& options,
                                      CreateStreamCallback* done = NULL) = 0;
    virtual int64_t CreateOutputStream(const StreamOptions& options,
                                       CreateStreamCallback* done = NULL) = 0;

    void CloseInputStream(int64_t stream_id, Closure<void>* callback)
    {
        CloseIStream(stream_id, ERROR_END_OF_STREAM, callback);
    }
    void CloseOutputStream(int64_t stream_id, Closure<void>* callback)
    {
        CloseOStream(stream_id, ERROR_END_OF_STREAM, callback);
    }
    void AbortInputStream(int64_t stream_id, Closure<void>* callback)
    {
        CloseIStream(stream_id, ERROR_STREAM_ABORTED, callback);
    }
    void AbortOutputStream(int64_t stream_id, Closure<void>* callback)
    {
        CloseOStream(stream_id, ERROR_STREAM_ABORTED, callback);
    }

    // For internal use. Remove a specified stream.
    void RemoveInputStream(int64_t stream_id);
    void RemoveOutputStream(int64_t stream_id);

    void WaitInputStreamClose(int64_t stream_id);
    void WaitOutputStreamClose(int64_t stream_id);
    void WaitAllStreamClose();
    TimerManager* mutable_timer_manager()
    {
        return &m_timer_manager;
    }

protected:
    virtual void CloseIStream(int64_t stream_id,
                              ErrorCode error_code,
                              Closure<void>* callback) = 0;
    virtual void CloseOStream(int64_t stream_id,
                              ErrorCode error_code,
                              Closure<void>* callback) = 0;

    typedef std::map<int64_t, scoped_refptr<InputStreamImpl> > InputStreamMap;
    typedef std::map<int64_t, scoped_refptr<OutputStreamImpl> > OutputStreamMap;

    // Timer manager, trigger read/write/close timeouts.
    TimerManager m_timer_manager;
    // Mutex to protect state of streams.
    SimpleMutex m_mutex;
    // Conditional variable to wait all streams to close.
    ConditionVariable m_cond;
    // Input streams.
    InputStreamMap  m_input_streams;
    // Output streams.
    OutputStreamMap m_output_streams;
};

// class ServerStreamManager
class ServerStreamManager : public StreamManager
{
public:
    ServerStreamManager() {}
    virtual ~ServerStreamManager();

    virtual int64_t CreateInputStream(const StreamOptions& options,
                                      CreateStreamCallback* done = NULL);
    virtual int64_t CreateOutputStream(const StreamOptions& options,
                                       CreateStreamCallback* done = NULL);

    // Handle the underlying rpc requests.
    void OnReceiveDownloadRequest(
        google::protobuf::RpcController* controller,
        const DownloadPacketRequest* request,
        DownloadPacketResponse* response,
        google::protobuf::Closure* done);

    void OnReceiveUploadPacket(
        google::protobuf::RpcController* controller,
        const UploadPacketRequest* request,
        UploadPacketResponse* response,
        google::protobuf::Closure* done);

    void OnReceiveCloseInputStreamRequest(
        google::protobuf::RpcController* controller,
        const CloseStreamRequest* request,
        CloseStreamResponse* response,
        google::protobuf::Closure* done);

    void OnReceiveCloseOutputStreamRequest(
        google::protobuf::RpcController* controller,
        const CloseStreamRequest* request,
        CloseStreamResponse* response,
        google::protobuf::Closure* done);
private:
    virtual void CloseIStream(int64_t stream_id,
                              ErrorCode error_code,
                              Closure<void>* callback);
    virtual void CloseOStream(int64_t stream_id,
                              ErrorCode error_code,
                              Closure<void>* callback);

    // Allocate an unique id for each stream
    int32_t m_last_stream_id;
};

// class ClientStreamManager
class ClientStreamManager : public StreamManager
{
public:
    explicit ClientStreamManager(
        StreamingService::Stub* streaming_service_stub,
        bool own_stub = false) :
        m_own_stub(own_stub),
        m_streaming_service_stub(streaming_service_stub)
    {
    }
    explicit ClientStreamManager(google::protobuf::RpcChannel* channel)
    {
        m_own_stub = true;
        m_streaming_service_stub = new StreamingService::Stub(channel);
    }
    virtual ~ClientStreamManager();

    virtual int64_t CreateInputStream(const StreamOptions& options,
                                      CreateStreamCallback* done = NULL);
    virtual int64_t CreateOutputStream(const StreamOptions& options,
                                       CreateStreamCallback* done = NULL);

    StreamingService::Stub* mutable_streaming_service_stub()
    {
        return m_streaming_service_stub;
    }

private:
    virtual void CloseIStream(int64_t stream_id,
                              ErrorCode error_code,
                              Closure<void>* callback);
    virtual void CloseOStream(int64_t stream_id,
                              ErrorCode error_code,
                              Closure<void>* callback);

    void CreateInputStreamComplete(RpcController* controller,
                                   CreateStreamRequest* request,
                                   CreateStreamResponse* response,
                                   CreateStreamCallback* done);
    void CreateOutputStreamComplete(RpcController* controller,
                                    CreateStreamRequest* request,
                                    CreateStreamResponse* response,
                                    CreateStreamCallback* done);

    bool m_own_stub;
    StreamingService::Stub* m_streaming_service_stub;
};

} // namespace streaming
} // namespace poppy

#endif // POPPY_STREAMING_STREAM_MANAGER_H
