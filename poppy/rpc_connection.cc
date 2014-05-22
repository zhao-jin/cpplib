// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: DongPing HUANG <dphuang@tencent.com>
// Created: 11/14/11
// Description:

#include "poppy/rpc_connection.h"

#include <limits>

#include "common/base/static_assert.h"
#include "common/net/http/string_packet.h"
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/this_thread.h"
#include "common/system/time/timestamp.h"
#include "common/system/timer/timer_manager.h"

#include "poppy/rpc_channel_impl.h"
#include "poppy/rpc_client.h"
#include "poppy/rpc_client_impl.h"
#include "poppy/rpc_error_code_info.pb.h"
#include "poppy/rpc_handler.h"
#include "poppy/rpc_option.pb.h"

#include "thirdparty/protobuf/descriptor.h"
#include "thirdparty/snappy/snappy.h"

namespace poppy {

namespace {

static const int64_t kUndispachedRequestConnectionId = -1;

// The size of the rpc packet header, which used to indicate the size of the
// whole rpc packet. Currently it's to uint32_t to indiciate the size of rpc
// meta and rpc message respectively.
static const size_t kRpcHeaderSize = 2 * sizeof(uint32_t); // NOLINT(runtime/sizeof)

static inline void HostToBufferUInt32(const uint32_t value, char* data)
{
    data[0] = static_cast<uint8_t>(value >> 24);
    data[1] = static_cast<uint8_t>(value >> 16);
    data[2] = static_cast<uint8_t>(value >> 8);
    data[3] = static_cast<uint8_t>(value);
}

} // anonymous namespace

RpcConnection::RpcConnection(RpcClientImpl* rpc_client_impl,
                             const stdext::shared_ptr<RpcChannelImpl>& channel_impl,
                             const std::string& address)
    : m_rpc_client_impl(rpc_client_impl),
      m_channel_impl(channel_impl),
      m_http_connection(NULL),
      m_address(address),
      m_timer_manager(channel_impl->GetTimerManager()),
      m_status(Disconnected),
      m_healthy(false),
      m_requests(rpc_client_impl, m_timer_manager),
      m_pending_builtin_count(0)
{
}

RpcConnection::~RpcConnection()
{
    {
        Mutex::Locker locker(m_mutex);

        if (m_status == Shutdown) {
            return;
        }

        ChangeStatus(Shutdown);
        // waiting for requests
        m_requests.CancelAll(RPC_ERROR_ALL_REQUEST_DISCARDED);
    }
    // waiting for timer
    while (true) {
        {
            Mutex::Locker locker(m_mutex);
            if (m_http_connection == NULL &&
                m_requests.IsEmpty()) {
                break;
            }
        }
        ThisThread::Sleep(1);
    }
}

bool RpcConnection::Connect()
{
    Mutex::Locker locker(m_mutex);
    if ((Status)m_status != Disconnected &&
        ((Status)m_status != ConnectError)) {
        CHECK(false) << "status: " << m_status << ", Status check failed";
    }

    // New handler will be taken over by http frame.
    RpcClientHandler* handler = new RpcClientHandler();
    handler->set_rpc_connection(this);

    netframe::NetFrame::EndPointOptions endpoint_options;
    stdext::shared_ptr<RpcChannelImpl> rpc_channel_impl = m_channel_impl.lock();
    if (rpc_channel_impl) {
        if (rpc_channel_impl->Options().has_tos()) {
            endpoint_options.Priority(rpc_channel_impl->Options().tos());
        }
        // update status first
        RpcConnection::Status prev_status = m_status;
        ChangeStatus(RpcConnection::Connecting);
        if (!m_rpc_client_impl->AsyncConnect(m_address,
                                             handler,
                                             endpoint_options,
                                             &m_http_connection) ) {
            // AsyncConnect might fail, handle it for retry. Possibly it needs a timer
            // as no other way to trigger the retry event.
            LOG(ERROR) << "Failed to connect to server: " << m_address;
            // restore prev status
            ChangeStatus(prev_status);
            return false;
        }
        VLOG(3) << "Connect, with http connection: "
            << reinterpret_cast<void*>(m_http_connection);

        return true;
    }

    LOG(ERROR) << "RpcChannelImpl already destruct";
    return false;
}

bool RpcConnection::Disconnect()
{
    Mutex::Locker locker(m_mutex);
    if (m_http_connection == NULL) {
        return false;
    }

    if (m_status == RpcConnection::Disconnecting)
        return false;

    ChangeStatus(RpcConnection::Disconnecting);
    m_http_connection->Close();
    return true;
}

void RpcConnection::OnNewSession(HttpConnection* http_connection)
{
    {
        Mutex::Locker locker(m_mutex);
        if (m_status != Connecting) {
            return;
        }

        ChangeStatus(Connected);
    }
    stdext::shared_ptr<RpcChannelImpl> rpc_channel_impl = m_channel_impl.lock();
    if (rpc_channel_impl) {
        rpc_channel_impl->ChangeStatus(this, Connecting, Connected);

        SendHealthRequest();
    }
}

void RpcConnection::OnClose(HttpConnection* http_connection, int error_code)
{
    RpcConnection::Status status = RpcConnection::Disconnected;
    RpcConnection::Status prev_status;
    {
        Mutex::Locker locker(m_mutex);
        if (m_http_connection == NULL) {
            LOG(ERROR) << "Invalid http connection"
                << ", server address: " << m_address
                << ", current status: " << m_status;
            if (m_status == RpcConnection::Disconnected ||
                m_status == RpcConnection::Shutdown)
                return;
        } else {
            if (http_connection->mutable_http_response()->status() == 403) {
                status = RpcConnection::NoAuth;
                m_requests.CancelAll(RPC_ERROR_NO_AUTH);
            } else {
                if (error_code != 0) {
                    status = RpcConnection::ConnectError;
                }
                m_requests.CancelAll(RPC_ERROR_CONNECTION_CLOSED);
            }
            VLOG(3) << "Close http connection: " << m_http_connection
                << ", error_code: " << error_code
                << ", error_reason: " << strerror(error_code);
            m_http_connection = NULL;
        }
        prev_status = m_status;
        if (prev_status != Shutdown) {
            ChangeStatus(status);
        }
    }

    if (prev_status != Shutdown) {
        stdext::shared_ptr<RpcChannelImpl> rpc_channel_impl = m_channel_impl.lock();
        if (rpc_channel_impl)
            rpc_channel_impl->ChangeStatus(this, prev_status, status);
    }
}

bool RpcConnection::SendRequest(RpcRequest* rpc_request, RpcErrorCode* error_code)
{
    {
        Mutex::Locker locker(m_mutex);
        if (m_http_connection == NULL ||
                (m_status != Connected && m_status != Healthy)) {
            if (error_code)
                *error_code = RPC_ERROR_CONNECTION_CLOSED;
            return false;
        }
        rpc_request->controller->set_remote_address(
            m_http_connection->GetRemoteAddress());
    }

    RpcMeta rpc_meta;
    rpc_meta.set_type(RpcMeta::REQUEST);
    rpc_meta.set_sequence_id(rpc_request->controller->sequence_id());
    rpc_meta.set_method(rpc_request->controller->method_full_name());
    rpc_meta.set_timeout(rpc_request->controller->Timeout());
    int expected_response_compress_type =
        rpc_request->controller->ResponseCompressType();
    if (expected_response_compress_type != CompressTypeAuto) {
        rpc_meta.set_expected_response_compress_type(
            static_cast<CompressType>(expected_response_compress_type));
    }

    std::string message_data;

    if (rpc_request->request) {
        CHECK(rpc_request->request->AppendToString(&message_data))
            << "Invalid request message";
    } else {
        message_data.assign(rpc_request->request_data);
    }

    CompressType request_compress_type =
        static_cast<CompressType>(rpc_request->controller->RequestCompressType());

    switch (request_compress_type) {
    case CompressTypeNone:
        break;
    case CompressTypeSnappy: {
            rpc_meta.set_compress_type(CompressTypeSnappy);
            std::string compressed_message;
            snappy::Compress(message_data.data(), message_data.size(),
                             &compressed_message);
            compressed_message.swap(message_data);
        }
        break;
    default:
        CHECK(!CompressType_IsValid(request_compress_type))
            << "Missed case branch in switch";
    }

    Mutex::Locker locker(m_mutex);

    if (!SendMessage(rpc_meta, message_data, error_code)) {
        return false;
    }

    int64_t expected_timeout =
        rpc_request->controller->Time() + rpc_request->controller->Timeout();
    m_requests.Add(rpc_request, expected_timeout, !rpc_request->IsBuiltin());

    return true;
}

bool RpcConnection::SendMessage(const RpcMeta& rpc_meta,
                                const StringPiece& message_data,
                                RpcErrorCode* error_code)
{
    if (m_http_connection == NULL ||
            (m_status != Connected && m_status != Healthy)) {
        if (error_code)
            *error_code = RPC_ERROR_CONNECTION_CLOSED;
        return false;
    }

    // The buffer is initialized with 8 bytes.
    std::string buffer;
    buffer.resize(kRpcHeaderSize);
    // Append content of rpc meta to the buffer.
    CHECK(rpc_meta.AppendToString(&buffer)) << "Invalid meta message";
    // Encode the size of rpc meta.
    HostToBufferUInt32(buffer.size() - kRpcHeaderSize, &buffer[0]);
    // Encode the size of rpc message.
    HostToBufferUInt32(message_data.size(),
                       &buffer[sizeof(uint32_t)]); // NOLINT(runtime/sizeof)
    // Append content of message to the buffer.
    // We have to send all data for one message in one packet to ensure they are
    // sent and received as a whole.
    buffer.append(message_data.data(), message_data.size());

    // Send the header and rpc meta data.
    if (!m_http_connection->SendPacket(new StringPacket(&buffer))) {
        // Failed to send packet, close the connection.
        LOG(INFO) << "Send packet error, close connection: "
                  << m_http_connection->GetConnectionId();
        ChangeStatus(RpcConnection::Disconnecting);
        m_http_connection->Close();
        if (error_code)
            *error_code = RPC_ERROR_SEND_BUFFER_FULL;
        return false;
    }

    return true;
}

void RpcConnection::HandleResponse(StringPiece meta, StringPiece data)
{
    RpcMeta rpc_meta;

    if (!rpc_meta.ParseFromArray(meta.data(), meta.length())) {
        // Meta is invalid, unexpected, close the connection.
#ifdef NDEBUG
        LOG(ERROR) << "Failed to parse the rpc meta, it is missing some required fields";
#else
        LOG(ERROR) << "Failed to parse the rpc meta, it is missing required fields: "
            << rpc_meta.InitializationErrorString();
#endif
        m_http_connection->Close();
        return;
    }

    if (rpc_meta.sequence_id() < 0) {
        // Sequence id is invalid, unexpected, close the connection.
        LOG(ERROR) << "Invalid rpc sequence id:" << rpc_meta.sequence_id();
        m_http_connection->Close();
        return;
    }

    VLOG(4) << "Recive response from connection: "
            << m_http_connection->GetConnectionId() << ", sequence_id :"
            << rpc_meta.sequence_id();

    uint64_t sequence_id = rpc_meta.sequence_id();
    RpcRequest* rpc_request = NULL;
    {
        Mutex::Locker locker(m_mutex);
        rpc_request = m_requests.RemoveAndConfirm(
                sequence_id,
                RpcWorkload::Response);
    }

    if (rpc_request == NULL) {
        // Possibly the session has been closed, just discard the received response.
        VLOG(3) << "Fail to get request from pending request queue."
                << "sequence id: " << sequence_id;
        return;
    }

    std::string uncompressed_message;

    if (rpc_meta.failed()) {
        // there is no error_code field in early RcpMeta, if the server is old
        // version, eror_code will be always 0(RPC_SUCCESS)
        int error_code = rpc_meta.error_code();
        if (error_code == RPC_SUCCESS) {
            error_code = RPC_ERROR_UNKNOWN;
            LOG(WARNING) << "Server " << m_http_connection->GetRemoteAddress().ToString()
                << " return failed but error_code equal zero,"
                << " maybe poppy version use in server is too old";
        }
        rpc_request->controller->SetFailed(error_code, rpc_meta.reason());
    } else {
        if (rpc_meta.has_compress_type()) {
            CompressType response_compress_type = rpc_meta.compress_type();

            switch (response_compress_type) {
            case CompressTypeNone:
                break;
            case CompressTypeSnappy:

                if (snappy::Uncompress(data.data(), data.size(),
                                       &uncompressed_message)) {
                    data.set(uncompressed_message.data(),
                             uncompressed_message.size());
                } else {
                    LOG(ERROR) << "Fail to uncompress the message data. "
                               << "sequence id: " << rpc_meta.sequence_id();
                    rpc_request->controller->SetFailed(RPC_ERROR_UNCOMPRESS_MESSAGE);
                }

                break;
            default:
                LOG(ERROR) << "Unknown compress type. sequence id: "
                           << "sequence id: " << rpc_meta.sequence_id();
                rpc_request->controller->SetFailed(RPC_ERROR_UNCOMPRESS_MESSAGE);
            }
        }
    }

    std::string request_method_name = rpc_request->controller->method_full_name();

    // Called from netframe thread when receiving response packet.
    rpc_request->done->Run(&data);

    static const std::string& builtin_service_name =
        BuiltinService::descriptor()->full_name();

    if (!StringStartsWith(request_method_name, builtin_service_name)) {
        m_requests.GetWorkload()->SetLastUseTime();
    }

    delete rpc_request;
}

void RpcConnection::SendHealthRequest() {
    poppy::RpcController* controller = new poppy::RpcController();
    poppy::EmptyRequest* request = new poppy::EmptyRequest();
    poppy::HealthResponse* response = new poppy::HealthResponse();
    Closure<void, const StringPiece*>* done = NewClosure(this,
            &RpcConnection::HealthCallback, controller, request, response);

    static const google::protobuf::MethodDescriptor* method
            = BuiltinService::descriptor()->FindMethodByName("Health");

    // for those request for heart beat, don't need to retry.
    controller->set_fail_immediately(true);
    controller->FillFromMethodDescriptor(method);

    stdext::shared_ptr<RpcChannelImpl> rpc_channel_impl = m_channel_impl.lock();
    if (rpc_channel_impl) {
        RpcRequest* rpc_request =
            rpc_channel_impl->CreateRequest(controller, request, NULL, done);
        rpc_request->SetBuiltin(true);
        AtomicIncrement(&m_pending_builtin_count);
        if (!SendRequest(rpc_request)) {
            delete controller;
            delete request;
            delete response;
            delete done;
            delete rpc_request;

            AtomicDecrement(&m_pending_builtin_count);
        }
    }
}

void RpcConnection::HealthCallback(poppy::RpcController* controller,
                                   poppy::EmptyRequest* request,
                                   poppy::HealthResponse* response,
                                   const StringPiece* message)
{
    HandleHealthChange(
        !controller->Failed() &&
        response->ParseFromArray(message->data(), message->size()) &&
        response->health() == "OK");

    delete controller;
    delete request;
    delete response;

    AtomicDecrement(&m_pending_builtin_count);
}

void RpcConnection::HandleHealthChange(bool healthy)
{
    Status prev_status = Disconnected;
    Status status = Disconnected;

    {
        Mutex::Locker locker(m_mutex);
        if (m_status != Connected && m_status != Healthy) {
            return;
        }

        prev_status = m_status;
        ChangeStatus(healthy ? Healthy : Connected);
        status = m_status;
    }

    stdext::shared_ptr<RpcChannelImpl> rpc_channel_impl = m_channel_impl.lock();
    if (rpc_channel_impl) {
        if (prev_status != status) {
            rpc_channel_impl->ChangeStatus(this, prev_status, m_status);
        }
        // if there are pending undispatched requests, it's time to send them
        // out now.
        if (healthy) {
            rpc_channel_impl->RedispatchRequests();
        }
    }
}

void RpcConnection::ChangeStatus(Status status)
{
    switch (m_status) {
    case RpcConnection::Disconnected:
    case RpcConnection::ConnectError:
        if (status == RpcConnection::Connecting) {
            m_status = status;
            return;
        }
        break;
    case RpcConnection::Disconnecting:
        if (status == RpcConnection::Disconnecting)
            return;
        if (status == RpcConnection::Disconnected ||
            status == RpcConnection::ConnectError) {
            m_status = status;
            return;
        }
        break;
    case RpcConnection::Connecting:
    case RpcConnection::Connected:
    case RpcConnection::Healthy:
        m_status = status;
        return;
    case RpcConnection::Shutdown:
        return;
    default:
        break;
    }

    if (status == Shutdown) {
        m_status = status;
        return;
    }

    LOG(FATAL) << "Rpc connection status transfer error - not allowed to "
        << "transfer to status " << status << " from " << m_status;
    return;
}

RpcConnection::Status RpcConnection::GetConnectStatus() const
{
    Mutex::Locker locker(m_mutex);
    return m_status;
}

uint64_t RpcConnection::GetPendingRequestsCount() const
{
    Mutex::Locker locker(m_mutex);
    return m_requests.GetPendingCount();
}

common::CredentialGenerator* RpcConnection::GetCredentialGenerator()
{
    stdext::shared_ptr<RpcChannelImpl> rpc_channel_impl = m_channel_impl.lock();
    if (rpc_channel_impl) {
        return rpc_channel_impl->GetCredentialGenerator();
    } else {
        LOG(ERROR) << "RpcChannelImpl already destruct";
        return NULL;
    }
}

const std::string& RpcConnection::GetAddress() const
{
    return m_address;
}

RpcWorkload* RpcConnection::GetWorkload()
{
    return m_requests.GetWorkload();
}

RpcChannelImpl* RpcConnection::GetRpcChannelImpl() const
{
    stdext::shared_ptr<RpcChannelImpl> rpc_channel_impl = m_channel_impl.lock();
    if (rpc_channel_impl) {
        return rpc_channel_impl.get();
    }
    return NULL;
}

} // namespace poppy

