// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/streaming/stream.h"
#include "poppy/streaming/stream_manager.h"
#include <string>

namespace poppy {
namespace streaming {

bool OutputStream::Create(const StreamOptions& options,
                          Closure<void, ErrorCode>* callback)
{
    CHECK_LT(m_stream_id, 0) << "Stream is already initialized with id: "
                             << m_stream_id;

    if (callback != NULL) {
        StreamManager::CreateStreamCallback* closure =
            NewClosure(this, &OutputStream::CreateComplete, callback);
        m_stream_manager->CreateOutputStream(options, closure);
        return true;
    } else {
        m_stream_id = m_stream_manager->CreateOutputStream(options, NULL);
        return m_stream_id > 0;
    }
}

void OutputStream::CreateComplete(Closure<void, ErrorCode>* callback,
                                  ErrorCode error_code, int64_t stream_id)
{
    m_stream_id = stream_id;
    callback->Run(error_code);
}

void OutputStream::Close(Closure<void>* callback)
{
    m_stream_manager->CloseOutputStream(m_stream_id, callback);
}

void OutputStream::Abort(Closure<void>* callback)
{
    m_stream_manager->AbortOutputStream(m_stream_id, callback);
}

void OutputStream::Write(const std::string* data, CompletionCallback* done)
{
    m_stream_manager->WritePacket(m_stream_id, data, done);
}

void OutputStream::Write(const google::protobuf::Message& message,
                         CompletionCallback* done)
{
    m_stream_manager->WriteMessage(m_stream_id, message, done);
}

void OutputStream::WaitUntilClose()
{
    m_stream_manager->WaitOutputStreamClose(m_stream_id);
}

bool InputStream::Create(const StreamOptions& options,
                         Closure<void, ErrorCode>* callback)
{
    CHECK_LT(m_stream_id, 0) << "Stream is already initialized with id: "
                             << m_stream_id;

    if (callback != NULL) {
        StreamManager::CreateStreamCallback* closure =
            NewClosure(this, &InputStream::CreateComplete, callback);
        m_stream_manager->CreateInputStream(options, closure);
        return true;
    } else {
        m_stream_id = m_stream_manager->CreateInputStream(options, NULL);
        return m_stream_id > 0;
    }
}

void InputStream::CreateComplete(Closure<void, ErrorCode>* callback,
                                 ErrorCode error_code, int64_t stream_id)
{
    m_stream_id = stream_id;
    callback->Run(error_code);
}

void InputStream::Close(Closure<void>* callback)
{
    m_stream_manager->CloseInputStream(m_stream_id, callback);
}

void InputStream::Abort(Closure<void>* callback)
{
    m_stream_manager->AbortInputStream(m_stream_id, callback);
}

void InputStream::Read(CompletionCallback* done)
{
    m_stream_manager->ReadPacket(m_stream_id, done);
}

void InputStream::Read(google::protobuf::Message* message,
                       CompletionCallback* done)
{
    m_stream_manager->ReadMessage(m_stream_id, message, done);
}

void InputStream::WaitUntilClose()
{
    m_stream_manager->WaitInputStreamClose(m_stream_id);
}

} // namespace streaming
} // namespace poppy
