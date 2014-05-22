// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef POPPY_STREAMING_STREAM_H
#define POPPY_STREAMING_STREAM_H

#include <string>
#include "common/base/stdint.h"
#include "poppy/streaming/streaming_service_info.pb.h"

namespace poppy {
namespace streaming {

// Forward declaration.
class StreamManager;

// Read/Write completion callback defination.
typedef ::Closure<void, ErrorCode, const std::string*> CompletionCallback;

// Stream Interface.
class Stream
{
public:
    // Constructor. The stream_id is -1 defaultly. On client side, you should
    // call Create() function to create a stream id initiatively. On server
    // side, the stream id is specified by client via rpc call. If stream id
    // has been assigned, it can't be changed or recreated.
    explicit Stream(StreamManager* stream_manager, int64_t stream_id = -1)
        : m_stream_manager(stream_manager), m_stream_id(stream_id)
    {
    }
    virtual ~Stream() {}

    int64_t stream_id() const
    {
        return m_stream_id;
    }

    // Create a stream if current stream is not initialized.
    // This function is used on the client side.
    // It's an asynchronous function if 'callback' is not NULL.
    // It's a synchronous function if 'callback' is NULL.
    virtual bool Create(const StreamOptions& options,
                        Closure<void, ErrorCode>* callback = NULL) = 0;

    // Close current stream. It's an asynchronous function no matter if the
    // callback closure is set. After this function is called, no data can
    // be read from or wrote to current stream.
    virtual void Close(Closure<void>* callback = NULL) = 0;

    // Abort current stream. It's an asynchronous function no matter if the
    // callback closure is set. After this function is called, no data can
    // be read from or wrote to current stream.
    // Abort a stream means abnormally close a stream. e.g., when some errors
    // occur on upper application, it uses Abort() to close current stream
    // without waiting all data are sent. The remote side will receive a notice
    // that the stream is closed unexpectedly.
    virtual void Abort(Closure<void>* callback = NULL) = 0;

    // This function is used for synchronous close.
    // A wait function. It will wait until all operations on the stream are
    // finished and the stream is really closed.
    virtual void WaitUntilClose() = 0;

protected:
    StreamManager* m_stream_manager;
    int64_t m_stream_id;
};

// Output Stream.
class OutputStream : public Stream
{
public:
    explicit OutputStream(StreamManager* stream_manager, int64_t stream_id = -1)
        : Stream(stream_manager, stream_id)
    {
    }
    virtual ~OutputStream() {}

    // Interface implementation.
    virtual bool Create(const StreamOptions& options,
                        Closure<void, ErrorCode>* callback = NULL);
    virtual void Close(Closure<void>* callback = NULL);
    virtual void Abort(Closure<void>* callback = NULL);
    virtual void WaitUntilClose();
    // Write data to current stream. Asynchronous function.
    // The callback function will indicate if the operation is successful or not.
    void Write(const std::string* data, CompletionCallback* done);
    void Write(const google::protobuf::Message& message, CompletionCallback* done);

protected:
    void CreateComplete(Closure<void, ErrorCode>* callback,
                        ErrorCode error_code,
                        int64_t stream_id);
};

// Input Stream.
class InputStream : public Stream
{
public:
    explicit InputStream(StreamManager* stream_manager, int64_t stream_id = -1)
        : Stream(stream_manager, stream_id)
    {
    }
    virtual ~InputStream() {}

    // Interface implementation.
    virtual bool Create(const StreamOptions& options,
                        Closure<void, ErrorCode>* callback = NULL);
    virtual void Close(Closure<void>* callback = NULL);
    virtual void Abort(Closure<void>* callback = NULL);
    virtual void WaitUntilClose();
    // Read data from current stream. Asynchronous function.
    // The callback function will indicate if the operation is successful or not.
    void Read(CompletionCallback* done);
    void Read(google::protobuf::Message* message, CompletionCallback* done);

protected:
    void CreateComplete(Closure<void, ErrorCode>* callback,
                        ErrorCode error_code,
                        int64_t stream_id);
};

} // namespace streaming
} // namespace poppy

#endif // POPPY_STREAMING_STREAM_H
