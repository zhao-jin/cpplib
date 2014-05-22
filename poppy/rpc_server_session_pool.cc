// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/rpc_server_session_pool.h"

#include <string>
#include "common/net/http/string_packet.h"
#include "common/system/concurrency/rwlock.h"
#include "poppy/rpc_controller.h"
#include "poppy/rpc_meta_info.pb.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/protobuf/descriptor.h"
#include "thirdparty/snappy/snappy.h"

namespace poppy {

// using namespace common;

namespace {

// The size of the rpc packet header, which used to indicate the size of the
// whole rpc packet. Currently it's to uint32_t to indiciate the size of rpc
// meta and rpc message respectively.
static const int kRpcHeaderSize = 2 * sizeof(uint32_t); // NOLINT(runtime/sizeof)

static inline void HostToBufferUInt32(const uint32_t value, char* data)
{
    data[0] = static_cast<uint8_t>(value >> 24);
    data[1] = static_cast<uint8_t>(value >> 16);
    data[2] = static_cast<uint8_t>(value >> 8);
    data[3] = static_cast<uint8_t>(value);
}

} // namespace

// class RpcServerSessionPool implementations
RpcServerSessionPool::~RpcServerSessionPool()
{
    CloseAllSessions();
}

void RpcServerSessionPool::CloseAllSessions()
{
    RWLock::WriterLocker locker(m_connection_rwlock);

    for (ConnectionMap::const_iterator i = m_rpc_sessions.begin();
         i != m_rpc_sessions.end();
         ++i) {
        i->second->Close();
    }
}

void RpcServerSessionPool::OnNewSession(HttpConnection* http_connection)
{
    // The connection is valid as a rpc communication channel.
    RWLock::WriterLocker locker(m_connection_rwlock);
    CHECK(m_rpc_sessions.insert(
            std::make_pair(http_connection->GetConnectionId(),
                           http_connection)).second)
        << "Duplicated connection id: " << http_connection->GetConnectionId();
}

void RpcServerSessionPool::OnClose(int64_t connection_id, int error_code)
{
    RWLock::WriterLocker locker(m_connection_rwlock);
    m_rpc_sessions.erase(connection_id);
}

void RpcServerSessionPool::SendResponse(
    const RpcController* controller,
    const google::protobuf::Message* response)
{
    CHECK_GE(controller->connection_id(), 0)
            << "The connection id of controller must be set at server side.";

    RpcMeta response_meta;

    RWLock::ReaderLocker locker(m_connection_rwlock);
    ConnectionMap::const_iterator it =
        m_rpc_sessions.find(controller->connection_id());

    // Process only if the session still exists.
    if (it != m_rpc_sessions.end()) {
        response_meta.set_type(RpcMeta::RESPONSE);
        response_meta.set_sequence_id(controller->sequence_id());

        if (controller->Failed()) {
            response_meta.set_failed(true);
            response_meta.set_error_code(controller->ErrorCode());
            response_meta.set_reason(controller->reason());

            if (controller->IsCanceled()) {
                response_meta.set_canceled(true);
            }
        }

        HttpConnection* http_connection = it->second;
        std::string message_data;
        if (!controller->Failed()) {
            DCHECK(response->IsInitialized())
                << "Invalid response message, it is missing required fields: "
                << response->InitializationErrorString();

            // Don't bother sending response data if failed.
            CHECK(response->AppendToString(&message_data))
                << "Invalid response message, it is missing some requird fields";

            CompressType response_compress_type =
                static_cast<CompressType>(controller->ResponseCompressType());
            if (response_compress_type != CompressTypeNone &&
                http_connection->peer_supported_compress_types().find(
                    response_compress_type) ==
                    http_connection->peer_supported_compress_types().end()) {
                response_compress_type = CompressTypeNone;
            }
            if (response_compress_type != CompressTypeNone) {
                response_meta.set_compress_type(response_compress_type);
            }

            switch (response_compress_type) {
            case CompressTypeNone:
                break;
            case CompressTypeSnappy: {
                    std::string compressed_message;
                    snappy::Compress(message_data.data(), message_data.size(),
                                     &compressed_message);
                    compressed_message.swap(message_data);
                }
                break;
            default:
                CHECK(!CompressType_IsValid(response_compress_type))
                    << "Missed case branch in switch";
            }
        }

        SendMessage(http_connection, response_meta, message_data);
    }
}

HttpConnection* RpcServerSessionPool::GetHttpConnection(int64_t connection_id)
{
    RWLock::ReaderLocker locker(m_connection_rwlock);
    ConnectionMap::const_iterator it =
        m_rpc_sessions.find(connection_id);
    return (it != m_rpc_sessions.end()) ? it->second : NULL;
}

// This function is already in protection of a lock.
void RpcServerSessionPool::SendMessage(HttpConnection* http_connection,
                                       const RpcMeta& rpc_meta,
                                       const StringPiece& message_data)
{
    // The buffer is initialized with 8 bytes.
    std::string buffer;
    buffer.resize(kRpcHeaderSize);
    DCHECK(rpc_meta.IsInitialized())
        << "Invalid rpc meta, it is missing required fields: "
        << rpc_meta.InitializationErrorString();
    // Append content of rpc meta to the buffer.
    CHECK(rpc_meta.AppendToString(&buffer))
        << "Invalid rpc meta, it is missing some required fields";
    // Encode the size of rpc meta.
    HostToBufferUInt32(buffer.size() - kRpcHeaderSize, &buffer[0]);
    // Encode the size of rpc message.
    HostToBufferUInt32(message_data.size(), &buffer[sizeof(uint32_t)]);
    // Append content of message to the buffer.
    // We have to send all data for one message in one packet to ensure they
    // are sent and received as a whole.
    buffer.append(message_data.data(), message_data.size());

    // Send the header and rpc meta data.
    if (!http_connection->SendPacket(new StringPacket(&buffer))) {
        // Failed to send packet, close the connection.
        LOG(INFO) << "Send packet error, close connection: "
                  << http_connection->GetConnectionId()
                  << ", sequence id: " << rpc_meta.sequence_id();
        http_connection->Close();
    } else {
        VLOG(4) << "Enqueue response complete, sequence id:"
                << rpc_meta.sequence_id();
    }
}

} // namespace poppy
