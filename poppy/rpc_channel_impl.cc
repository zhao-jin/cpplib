// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
// Dongping HUANG <dphuang@tencent.com>

#include "poppy/rpc_channel_impl.h"

#include <algorithm>

#include "common/crypto/random/pseudo_random.h"
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/rwlock.h"
#include "common/system/process.h"
#include "common/system/time/timestamp.h"
#include "common/system/timer/timer_manager.h"

#include "poppy/rpc_channel.h"
#include "poppy/rpc_client_impl.h"
#include "poppy/rpc_controller.h"
#include "poppy/rpc_handler.h"
#include "poppy/rpc_message.pb.h"
#include "poppy/rpc_option.pb.h"
#include "poppy/rpc_request_queue.h"

#include "protobuf/descriptor.h"
#include "snappy/snappy.h"
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

// poppy default tos value is 96 (011-000-00)
// if we use poppy default tos value, poppy will use WAN to transfer
// data if two communication endpoints are in different IDCs.
DEFINE_int32(poppy_default_tos, 96, "poopy default tos value");

namespace {

// If server list's number is greater than kServerToConnectLowerBound,
// poppy at least maintains kServerToConnectLowerBound connections to
// different server, otherwise, it maintains connection to each server
// in server list
const uint32_t kServerToConnectLowerBound = 3;

uint32_t GetRand(uint32_t begin, uint32_t end) {
    static PseudoRandom rand_gen(static_cast<uint64_t>(ThisProcess::GetId()));
    CHECK_GT(end, begin) << "Invalid range for rand generator";
    return rand_gen.NextUInt32() % (end - begin) + begin;
}

} // anonymous namespace

namespace poppy {

DEPRECATED_BY(::FLAGS_poppy_default_tos)
::google::int32& FLAGS_poppy_default_tos = ::FLAGS_poppy_default_tos;

RpcChannelImpl::RpcChannelImpl(const stdext::shared_ptr<RpcClientImpl>& rpc_client_impl,
                               const std::string& name,
                               uint64_t hash,
                               const std::vector<std::string>& servers,
                               common::CredentialGenerator* credential_generator,
                               const RpcChannelOptions& options) :
    m_channel_name(name),
    m_hash(hash),
    m_rpc_client_impl(rpc_client_impl),
    m_server_addresses(servers),
    m_current_event(ChannelEvent_Disconnect),
    m_connect_status(RpcConnection::Disconnected),
    m_last_sequence_id(0),
    m_timer_manager(rpc_client_impl->mutable_timer_manager()),
    m_requests(rpc_client_impl.get(), m_timer_manager),
    m_timer_id(0),
    m_last_check_error_time(0),
    m_timer_connect(0),
    m_credential_generator(credential_generator),
    m_options(options),
    m_pending_request_count(0)
{
    if (m_credential_generator == NULL) {
        common::CredentialGenerator* default_generator =
            common::CredentialGenerator::DefaultInstance();
        m_credential_generator = (default_generator ? default_generator :
                                  &common::DummyCredentialGenerator::Instance());
    }
    if (m_options.has_tos()) {
        CHECK(m_options.tos() > 0 && m_options.tos() < 256)
            << "Invalid tos option value";
    } else {
        m_options.set_tos(FLAGS_poppy_default_tos);
    }
    if (m_options.has_keepalive_time()) {
        CHECK(m_options.keepalive_time() > 0 || m_options.keepalive_time() == -1)
            << "Invalid keepalive_time option value";
    }
    if (m_options.has_connect_timeout()) {
        CHECK_GT(m_options.connect_timeout(), 0)
            << "Invalid connect_timeout option value";
    }

    srand(static_cast<unsigned int>(ThisProcess::GetId()));
    std::random_shuffle(m_server_addresses.begin(), m_server_addresses.end());

    m_requests.GetWorkload()->SetNext(&m_workload);
}

RpcChannelImpl::~RpcChannelImpl()
{
    VLOG(3) << m_workload.Dump();
}

void RpcChannelImpl::AddAllConnections()
{
    // create rpc connections and insert them into disconnected queue.
    for (std::vector<std::string>::iterator it = m_server_addresses.begin();
        it != m_server_addresses.end();
        ++it) {
        AddConnection(*it);
    }
}

void RpcChannelImpl::Shutdown(bool wait_all_pending_request_finish)
{
    ChannelEvent current_event = m_current_event;
    if (current_event == ChannelEvent_Shutdowning ||
        current_event == ChannelEvent_Shutdown) {
        return;
    }

    {
        RWLock::WriterLocker locker(m_connection_rwlock);
        current_event = m_current_event;
        if (current_event == ChannelEvent_Shutdowning ||
            current_event == ChannelEvent_Shutdown) {
            return;
        }

        m_current_event = ChannelEvent_Shutdowning;
    }

    if (wait_all_pending_request_finish) {
        LOG(INFO) << "Waiting for "
                  << AtomicGet(&m_pending_request_count)
                  << " pending requests to finish";
        while (AtomicGet(&m_pending_request_count) > 0) {
            ThisThread::Sleep(1);
            LOG_EVERY_N(INFO, 1000) << "Waiting for "
                                    << AtomicGet(&m_pending_request_count)
                                    << " pending requests to finish";
        }
    }

    {
        RWLock::WriterLocker locker(m_connection_rwlock);
        m_current_event = ChannelEvent_Shutdown;
        Close();
        m_requests.Shutdown();
    }

    {
        uint64_t start_time = GetTimeStampInMs();
        while (true) {
            {
                RWLock::ReaderLocker locker(m_connection_rwlock);
                if (m_timer_connect == 0 && m_timer_id == 0 &&
                    AllConnectionsClosed()) {
                    break;
                }

                int64_t current = GetTimeStampInMs();
                if (current - start_time >= 60 * 1000) {
                    LOG_EVERY_N(ERROR, 1000)
                        << "Can not shutdown in " << current - start_time
                        << " microseconds, make sure no any poppy client thread"
                        << " blocked. "
                        << "Sorry for the inconvenience, please contact poppy "
                        << "developers. Infomation: "
                        << "m_timer_connect=" << m_timer_connect << ", "
                        << "m_timer_id=" << m_timer_id
                        << ", healthy_connctions_count="
                        << m_rpc_connections[RpcConnection::Healthy].size()
                        << ", connected_connections_count="
                        << m_rpc_connections[RpcConnection::Connected].size()
                        << ", connection_connections_count="
                        << m_rpc_connections[RpcConnection::Connecting].size()
                        << ", disconnecting_connections_count="
                        << m_rpc_connections[RpcConnection::Disconnecting].size();
                }
            }

            ThisThread::Sleep(1);
        }
    }

    {
        RWLock::WriterLocker locker(m_connection_rwlock);

        for (int k = 0; k < RpcConnection::TotalStatusType; ++k) {
            std::vector<RpcConnection*>::iterator it;
            for (it = m_rpc_connections[k].begin();
                 it != m_rpc_connections[k].end();
                 ++it) {
                delete *it;
            }
            m_rpc_connections[k].clear();
        }
    }
}

void RpcChannelImpl::MakeConnectionWithDelay(int64_t delay)
{
    RWLock::WriterLocker locker(m_connection_rwlock);

    if (m_current_event == ChannelEvent_Shutdown) {
        return;
    }

    if (m_timer_connect > 0) {
        return;
    }

    m_current_event = ChannelEvent_Connect;

    TimerManager::CallbackClosure *closure = NewClosure(
            this, &RpcChannelImpl::DoMakeConnectionWithDelay,
            stdext::weak_ptr<RpcChannelImpl>(shared_from_this()));
    m_timer_connect = m_timer_manager->AddOneshotTimer(delay, closure);
}

void RpcChannelImpl::DoMakeConnectionWithDelay(stdext::weak_ptr<RpcChannelImpl> rpc_channel_impl,
                                               uint64_t timer_id)
{
    stdext::shared_ptr<RpcChannelImpl> rpc_channel = rpc_channel_impl.lock();
    if (!rpc_channel) {
        return;
    }

    RWLock::WriterLocker locker(m_connection_rwlock);

    m_timer_connect = 0;

    if (m_current_event != ChannelEvent_Connect) {
        return;
    }

    StartTimer();

    // If RpcConnection::Disconnected size is greater than
    // kServerToConnectLowerBound, poppy at least creates
    // kServerToConnectLowerBound connections to different server
    // in one DoMakeConnection call, otherwise, it creates connection
    // to each server in m_rpc_connections[RpcConnection::Disconnected]
    size_t size = m_rpc_connections[RpcConnection::Disconnected].size();
    if (size == 0) {
        return;
    }
    uint32_t connect_server_num = 0;
    do {
        uint32_t index = GetRand(0, size);
        RpcConnection* connection = m_rpc_connections[RpcConnection::Disconnected][index];
        ChangeStatusWithoutLock(connection,
                                RpcConnection::Disconnected,
                                RpcConnection::Connecting);
        if (connection->Connect()) {
            m_workload.SetLastUseTime();
            ++connect_server_num;
        } else {
            ChangeStatusWithoutLock(connection,
                                    RpcConnection::Connecting,
                                    RpcConnection::Disconnected);
            std::swap(m_rpc_connections[RpcConnection::Disconnected][index],
                      m_rpc_connections[RpcConnection::Disconnected][size - 1]);
        }
        --size;
    } while (size != 0 && connect_server_num < kServerToConnectLowerBound);
}

void RpcChannelImpl::AddConnection(const std::string& server)
{
    stdext::shared_ptr<RpcClientImpl> rpc_client = m_rpc_client_impl.lock();
    if (!rpc_client)
        return;

    RpcConnection *connection
        = new RpcConnection(rpc_client.get(), shared_from_this(), server);

    RpcWorkload* workload = connection->GetWorkload();
    workload->SetNext(&m_workload);

    RWLock::WriterLocker locker(m_connection_rwlock);
    m_rpc_connections[RpcConnection::Disconnected].push_back(connection);
}

void RpcChannelImpl::RemoveConnection(const std::string& server)
{
    RpcConnection* connection = NULL;
    {
        RWLock::WriterLocker locker(m_connection_rwlock);
        for (int k = 0; k < RpcConnection::TotalStatusType; ++k) {
            std::vector<RpcConnection*>::iterator it;
            for (it = m_rpc_connections[k].begin();
                 it != m_rpc_connections[k].end();
                 ++it) {
                if (server == (*it)->GetAddress()) {
                    connection = *it;
                    connection->Disconnect();
                    m_rpc_connections[k].erase(it);
                    break;
                }
            }
        }
    }
    delete connection;
}

void RpcChannelImpl::Close()
{
    assert(m_connection_rwlock.IsLocked());

    // Stop the period timer.
    StopTimer();

    RpcConnection::Status status[] = {
        RpcConnection::Healthy,
        RpcConnection::Connected,
        RpcConnection::Connecting
    };
    std::vector<RpcConnection*>::iterator it;
    for (uint32_t k = 0; k < sizeof(status)/sizeof(status[0]); ++k) {
        RpcConnection::Status s = status[k];
        // A copy of vector is used here. Because the RpcConnection::Disconnect
        // may fail because other thread from network has just notified that
        // the connection has just be closed. In this case, let it be, after
        // this thread return from Close, the network thread will get the lock
        // and it is responsible for moving the connection into disconnected
        // group.
        // For the thread calling Close, it could not operate on the original
        // copy directly, because in this case, the Disconnect call will fail
        // definitely, and it is not allowed to set the raw copy into an in-
        // consistent state.
        std::vector<RpcConnection*> connections = m_rpc_connections[s];
        for (std::vector<RpcConnection*>::iterator it = connections.begin();
             it != connections.end();
             ++it) {
            RpcConnection* connection = *it;
            ChangeStatusWithoutLock(connection,
                                    s,
                                    RpcConnection::Disconnecting);
            if (!connection->Disconnect()) {
                ChangeStatusWithoutLock(connection,
                                        RpcConnection::Disconnecting,
                                        s);
            }
        }
    }
}

RpcRequest* RpcChannelImpl::CreateRequest(
    RpcController* controller,
    const google::protobuf::Message* request,
    const std::string* request_data,
    Closure<void, const StringPiece*>* done)
{
    // Allocate a unique sequence id by increasing a counter monotonously.
    int64_t sequence_id =
        AtomicExchangeAdd(&m_last_sequence_id, static_cast<uint64_t>(1));
    controller->set_sequence_id(sequence_id);
    controller->set_time(GetTimeStampInMs());

    RpcRequest* rpc_request = new RpcRequest(sequence_id,
            controller,
            request,
            request_data ? *request_data : std::string(),
            done);
    return rpc_request;
}

RpcConnection* RpcChannelImpl::SelectAvailableConnection() const {
    size_t size = m_rpc_connections[RpcConnection::Healthy].size();
    if (size == 0) {
        return NULL;
    }

    uint32_t start = GetRand(0, size);
    uint32_t k = 0;
    RpcConnection *target = NULL;
    do {
        uint32_t index = (start + k) % size;
        RpcConnection* current = m_rpc_connections[RpcConnection::Healthy][index];

        if (current->GetConnectStatus() == RpcConnection::Healthy) {
            target = current;
            break;
        }

        ++k;
    } while ((start + k) % size != start);

    return target;
}

void RpcChannelImpl::RedispatchRequests()
{
    RWLock::ReaderLocker locker(m_connection_rwlock);

    if (m_current_event == ChannelEvent_Shutdown) {
        return;
    }

    while (true) {
        RpcConnection* connection = SelectAvailableConnection();

        if (!connection) {
            return;
        }

        RpcRequest* rpc_request = m_requests.PopFirst();

        if (rpc_request == NULL) {
            break;
        }

        RpcErrorCode error_code = RPC_SUCCESS;
        if (!connection->SendRequest(rpc_request, &error_code)) {
            rpc_request->controller->SetFailed(error_code);
            rpc_request->done->Run(NULL);
            delete rpc_request;
        }
    }
}

void RpcChannelImpl::SendRequest(
    RpcController* controller,
    const google::protobuf::Message* request,
    const std::string* request_data,
    Closure<void, const StringPiece*>* done)
{
    CHECK(!controller->in_use())
        << "The controller is IN USE. It holds important information for request, "
        << "and only can work for a single request at any given time. Don't reuse "
        << "it before the previous request finishes.";

    controller->set_in_use(true);

    Closure<void, const StringPiece*>* wrap_done =
        NewClosure(this, &RpcChannelImpl::DecrementPendingRequestCount, done);
    RpcRequest* rpc_request =
        CreateRequest(controller, request, request_data, wrap_done);

    RpcConnection* connection = NULL;
    bool need_connection = false;
    {
        RWLock::ReaderLocker locker(m_connection_rwlock);
        if (IsStoped()) {
            AbortRequest(controller, done);
            delete rpc_request;
            return;
        }

        AtomicIncrement(&m_pending_request_count);
        connection = SelectAvailableConnection();
        if (connection == NULL || !connection->SendRequest(rpc_request)) {
            // No available session now, add as an undispatched request.
            int64_t connect_timeout = GetTimeStampInMs() +
                m_options.connect_timeout();
            int64_t request_timeout = rpc_request->controller->Time() +
                rpc_request->controller->Timeout();
            int64_t expected_timeout = (std::min)(connect_timeout, request_timeout);
            m_requests.Add(rpc_request, expected_timeout);
            need_connection = true;
        }
    }
    if (need_connection) {
        MakeConnectionWithDelay(0);
    }
}

void RpcChannelImpl::InternalCallMethod(
    RpcController* controller,
    const google::protobuf::Message* request,
    const std::string* request_data,
    Closure<void, const StringPiece*>* done)
{
    if (controller->is_sync()) {
        AutoResetEvent event;
        Closure<void, const StringPiece*>* wrap_done =
            NewClosure(this, &RpcChannelImpl::SyncCallback, &event, done);
        SendRequest(controller, request, request_data, wrap_done);
        event.Wait();
    } else {
        SendRequest(controller, request, request_data, done);
    }
}

void RpcChannelImpl::CallMethod(
    RpcController* controller,
    const google::protobuf::Message* request,
    Closure<void, const StringPiece*>* done)
{
    CHECK_LE(request->ByteSize(), static_cast<int>(kMaxRequestSize))
        << controller->method_full_name() << ": Request too large";
    InternalCallMethod(controller, request, NULL, done);
}

void RpcChannelImpl::RawCallMethod(
    RpcController* controller,
    const std::string* request_data,
    Closure<void, const StringPiece*>* done)
{
    CHECK_LE(request_data->size(), kMaxRequestSize)
        << controller->method_full_name() << ": Request too large";
    InternalCallMethod(controller, NULL, request_data, done);
}

void RpcChannelImpl::SyncCallback(AutoResetEvent* event,
                                  Closure<void, const StringPiece*>* done,
                                  const StringPiece* message_data)
{
    done->Run(message_data);
    event->Set();
}

void RpcChannelImpl::DecrementPendingRequestCount(
    Closure<void, const StringPiece*>* done,
    const StringPiece* message_data)
{
    done->Run(message_data);
    AtomicDecrement(&m_pending_request_count);
}

bool RpcChannelImpl::HasActiveConnection() const
{
    return !m_rpc_connections[RpcConnection::Healthy].empty() ||
        !m_rpc_connections[RpcConnection::Connected].empty() ||
        !m_rpc_connections[RpcConnection::Connecting].empty();
}

bool RpcChannelImpl::AllConnectionsClosed() const
{
    return !HasActiveConnection() &&
        m_rpc_connections[RpcConnection::Disconnecting].empty();
}

ChannelStatus RpcChannelImpl::GetChannelStatus()
{
    if (IsUnrecoverable()) {
        return TranslateChannelStatus(m_connect_status);
    }

    bool reconnect = false;
    {
        RWLock::ReaderLocker locker(m_connection_rwlock);
        if (!HasActiveConnection()) {
            reconnect = true;
        }
    }
    if (reconnect) {
        MakeConnectionWithDelay(0);
    }

    m_connect_status = GetConnectStatus();
    ChannelStatus status = TranslateChannelStatus(m_connect_status);

    return status;
}

void RpcChannelImpl::OnAddressesChanged(
    const std::vector<std::string>& addresses)
{
    std::vector<std::string> servers = addresses;
    std::sort(servers.begin(), servers.end());

    std::vector<std::string> add_list;
    std::vector<std::string> remove_list;

    // diff the old and new address list.
    {
        RWLock::WriterLocker locker(m_connection_rwlock);

        if (IsUnrecoverable()) {
            return;
        }
        std::sort(m_server_addresses.begin(), m_server_addresses.end());

        std::vector<std::string>::iterator it1 = servers.begin();
        std::vector<std::string>::iterator it2 = m_server_addresses.begin();

        while (it1 != servers.end() && it2 != m_server_addresses.end()) {
            if (*it1 == *it2) {
                ++it1;
                ++it2;
                continue;
            } else if (*it1 < *it2) {
                add_list.push_back(*it1++);
            } else {
                remove_list.push_back(*it2++);
            }
        }
        add_list.insert(add_list.end(), it1, servers.end());
        remove_list.insert(remove_list.end(), it2, m_server_addresses.end());
        m_server_addresses.swap(servers);
        std::random_shuffle(m_server_addresses.begin(), m_server_addresses.end());
    }

    for (std::vector<std::string>::iterator it = add_list.begin();
         it != add_list.end();
         ++it) {
        AddConnection(*it);
    }

    for (std::vector<std::string>::iterator it = remove_list.begin();
         it != remove_list.end();
         ++it) {
        RemoveConnection(*it);
    }
}

bool RpcChannelImpl::ChangeStatus(RpcConnection* connection,
                                  RpcConnection::Status prev,
                                  RpcConnection::Status status)
{
    bool ret = false;
    bool need_connection = false;
    {
        RWLock::WriterLocker locker(m_connection_rwlock);
        ret = ChangeStatusWithoutLock(connection, prev, status);
        if (IsUnrecoverable()) {
            RpcErrorCode error = TranslateTimeoutErrorCode(m_connect_status);
            VLOG(10) << "Rpc error: status=" << m_connect_status
                     << " ErrorCode=" << RpcErrorCode_Name(error);
            m_requests.CancelAll(error);
        } else if (m_current_event == ChannelEvent_Connect && !HasActiveConnection()) {
            need_connection = true;
        }
    }
    if (need_connection) {
        MakeConnectionWithDelay(100);
    }
    return ret;
}

bool RpcChannelImpl::ChangeStatusWithoutLock(
    RpcConnection* connection,
    RpcConnection::Status prev,
    RpcConnection::Status status)
{
    std::vector<RpcConnection*>::iterator it;
    it = std::find(m_rpc_connections[prev].begin(),
                   m_rpc_connections[prev].end(),
                   connection);
    if (it == m_rpc_connections[prev].end()) {
        return false;
    }

    m_rpc_connections[prev].erase(it);
    m_rpc_connections[status].push_back(connection);

    m_connect_status = GetConnectStatusWithoutLock();
    m_requests.SetDefaultErrorCode(TranslateTimeoutErrorCode(m_connect_status));

    return true;
}

void RpcChannelImpl::CheckStatus(stdext::weak_ptr<RpcChannelImpl> rpc_channel_impl,
                                 uint64_t timer_id)
{
    stdext::shared_ptr<RpcChannelImpl> rpc_channel = rpc_channel_impl.lock();
    if (!rpc_channel) {
        return;
    }
    uint64_t current = GetTimeStampInMs();
    {
        RWLock::WriterLocker locker(m_connection_rwlock);

        m_timer_id = 0;

        if (m_current_event == ChannelEvent_Shutdowning ||
            m_current_event == ChannelEvent_Shutdown) {
            return;
        }

        StartTimer();

        uint64_t keepalive_time =
            static_cast<uint64_t>(m_options.keepalive_time());
        if (keepalive_time > 0 &&
            current - m_workload.GetLastUseTime() >= keepalive_time &&
            m_workload.GetPendingCount() == 0) {
            m_current_event = ChannelEvent_Disconnect;
            Close();
            return;
        }
    }

    bool has_undispatched_request = false;
    bool has_health_connection = false;
    bool has_overloaded_connection = false;
    bool need_try_error_connection = false;
    size_t error_connection_size = 0;
    {
        RWLock::ReaderLocker locker(m_connection_rwlock);
        if (m_requests.GetPendingCount() > 0) {
            has_undispatched_request = true;
        }
        error_connection_size = m_rpc_connections[RpcConnection::ConnectError].size();
        if (!m_rpc_connections[RpcConnection::Healthy].empty()) {
            has_health_connection = true;
            if (m_rpc_connections[RpcConnection::Healthy].size() !=
                m_server_addresses.size() &&
                m_rpc_connections[RpcConnection::Disconnected].size() != 0) {
                for (size_t index = 0;
                     index < m_rpc_connections[RpcConnection::Healthy].size();
                     ++index) {
                    RpcConnection* connection =
                        m_rpc_connections[RpcConnection::Healthy][index];
                    if (connection->GetPendingRequestsCount() > 2) {
                        has_overloaded_connection = true;
                        break;
                    }
                }
            }
        }
    }

    {
        RWLock::WriterLocker locker(m_connection_rwlock);
        if (error_connection_size != 0) {
            if (current - m_last_check_error_time >= 5000 ||
                (m_rpc_connections[RpcConnection::Healthy].empty()
                 && m_rpc_connections[RpcConnection::Disconnected].size() == 0)) {
                std::vector<RpcConnection*>::iterator it;
                for (it = m_rpc_connections[RpcConnection::ConnectError].begin();
                     it != m_rpc_connections[RpcConnection::ConnectError].end();
                     ++it) {
                    m_rpc_connections[RpcConnection::Disconnected].push_back(*it);
                }
                m_rpc_connections[RpcConnection::ConnectError].clear();
                need_try_error_connection = true;
                m_last_check_error_time = current;
            }
        }
    }

    SendHealthRequests();

    bool has_make_connection = false;
    if (has_undispatched_request) {
        if (has_health_connection) {
            RedispatchRequests();
        } else {
            MakeConnectionWithDelay(0);
            has_make_connection = true;
        }
    }
    if (!has_make_connection &&
        (has_overloaded_connection || need_try_error_connection)) {
        MakeConnectionWithDelay(0);
    }
}

void RpcChannelImpl::SendHealthRequests()
{
    RWLock::ReaderLocker locker(m_connection_rwlock);
    // send health heartbeat request
    RpcConnection::Status status[] = {
        RpcConnection::Healthy,
        RpcConnection::Connected
    };
    for (uint32_t k = 0; k < sizeof(status) / sizeof(status[0]); ++k) {
        std::vector<RpcConnection*>::iterator it;
        RpcConnection::Status s = status[k];
        for (it = m_rpc_connections[s].begin();
             it != m_rpc_connections[s].end();
             ++it) {
            // send a request to get health information.
            (*it)->SendHealthRequest();
        }
    }
}

void RpcChannelImpl::AbortRequest(RpcController *controller,
        Closure<void, const StringPiece*>* done)
{
    RpcErrorCode error_code = RPC_SUCCESS;
    if (m_current_event == ChannelEvent_Shutdown) {
        error_code = RPC_ERROR_CHANNEL_SHUTDOWN;
    } else {
        error_code = (m_connect_status == RpcConnection::NoAuth ?
            RPC_ERROR_NO_AUTH : RPC_ERROR_CHANNEL_SHUTDOWN);
    }
    controller->SetFailed(error_code);
    done->Run(NULL);
}

RpcConnection::Status RpcChannelImpl::GetConnectStatus()
{
    // the value cached in m_connect_status is not precise, update it before
    // return its value
    RWLock::ReaderLocker locker(m_connection_rwlock);
    m_connect_status = GetConnectStatusWithoutLock();
    return m_connect_status;
}

RpcConnection::Status RpcChannelImpl::GetConnectStatusWithoutLock() const
{
    for (int k = 0;
         k < RpcConnection::TotalStatusType;
         ++k) {
        if (!m_rpc_connections[k].empty()) {
            return (RpcConnection::Status)k;
        }
    }

    return RpcConnection::Disconnected;
}

bool RpcChannelImpl::IsStoped() const
{
    return m_current_event >= ChannelEvent_Shutdowning ||
           m_connect_status >= RpcConnection::Unrecoverable;
}

bool RpcChannelImpl::IsUnrecoverable() const
{
    return m_current_event == ChannelEvent_Shutdown ||
           m_connect_status >= RpcConnection::Unrecoverable;
}

void RpcChannelImpl::StartTimer()
{
    assert(m_connection_rwlock.IsLocked());
    // Start the period timer if neccessary.
    // This only happens for the first succeed connection.
    if (m_timer_id == 0) {
        TimerManager::CallbackClosure* timer_callback =
            NewClosure(this,
                       &RpcChannelImpl::CheckStatus,
                       stdext::weak_ptr<RpcChannelImpl>(shared_from_this()));
        // The period is 1s.
        m_timer_id = m_timer_manager->AddOneshotTimer(1000, timer_callback);
    }
}

void RpcChannelImpl::StopTimer()
{
    assert(m_connection_rwlock.IsLocked());
    if (m_timer_id != 0 &&
        m_timer_manager->AsyncRemoveTimer(m_timer_id)) {
        m_timer_id = 0;
    }
    if (m_timer_connect != 0 &&
        m_timer_manager->AsyncRemoveTimer(m_timer_connect)) {
        m_timer_connect = 0;
    }
}

ChannelStatus
RpcChannelImpl::TranslateChannelStatus(RpcConnection::Status status)
{
    switch (status) {
    case RpcConnection::Healthy:
        return ChannelStatus_Healthy;
    case RpcConnection::Connected:
        return ChannelStatus_Unavailable;
    case RpcConnection::NoAuth:
        return ChannelStatus_NoAuth;
    case RpcConnection::Shutdown:
        return ChannelStatus_Shutdown;
    default:
        return ChannelStatus_Unknown;
    }
}

RpcErrorCode
RpcChannelImpl::TranslateTimeoutErrorCode(RpcConnection::Status status)
{
    // Dot add default lable in the following switch statement.
    // We need compiler capture error of enum value not handled.
    switch (status) {
    case RpcConnection::Healthy:
        return RPC_ERROR_REQUEST_TIMEOUT;
    case RpcConnection::Connected:
        return RPC_ERROR_SERVER_UNAVAILABLE;
    case RpcConnection::NoAuth:
        return RPC_ERROR_NO_AUTH;
    case RpcConnection::ConnectError:
        return RPC_ERROR_SERVICE_UNREACHABLE;
    case RpcConnection::Connecting:
    case RpcConnection::Disconnecting:
    case RpcConnection::Disconnected:
        return RPC_ERROR_NETWORK_UNREACHABLE;
    case RpcConnection::Shutdown:
        return RPC_ERROR_CHANNEL_SHUTDOWN;
    case RpcConnection::TotalStatusType:
    case RpcConnection::Unrecoverable:
        // They are not real state.
        break;
    }
    LOG(FATAL) << "Invalid status, Should not reach here";
    return RPC_ERROR_NETWORK_UNREACHABLE;
}

} // namespace poppy
