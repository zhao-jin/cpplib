// Copyright 2011, Tencent Inc.
// Author: HUANG Dongping <dphuang@tencent.com>
// Created: 05/24/11
// Description:

#include "poppy/rpc_client_impl.h"

#include <algorithm>
#include <vector>

#include "common/base/closure.h"
#include "common/crypto/hash/city.h"
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/uuid/uuid.h"

#include "poppy/address_resolver/resolver_factory.h"
#include "poppy/rpc_channel_impl.h"
#include "poppy/rpc_http_channel.h"

#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

DEFINE_int32(poppy_client_work_thread_number, 4,
             "Number of network I/O threads for poppy client");

DEFINE_int32(poppy_client_callback_thread_number, 8,
             "Number of callback threads in thread pool to execute async "
             "callback of user for poppy client");

DEFINE_int32(poppy_client_channel_cache_size,
             poppy::RpcClient::kDefaultChannelCacheSize,
             "Size of RPC channel cache for poppy client");

DEFINE_int32(poppy_resolve_address_retry_times,
             3,
             "Number of retry times when poppy resolve address");

namespace poppy {

#define DEFINE_DEPRECATED_VARIABLE(type, name, new_name) \
    DEPRECATED_BY(::FLAGS_##new_name) type& FLAGS_##name = ::FLAGS_##new_name

#define DEFINE_deprecated_int32(name) \
    DEFINE_DEPRECATED_VARIABLE(::google::int32, name, name)

DEFINE_deprecated_int32(poppy_client_work_thread_number);
DEFINE_deprecated_int32(poppy_client_callback_thread_number);
DEFINE_deprecated_int32(poppy_client_channel_cache_size);
DEFINE_deprecated_int32(poppy_resolve_address_retry_times);

RpcClientImpl::RpcClientImpl(const RpcClientOptions& options,
                             netframe::NetFrame* netframe) :
    HttpClient(netframe, options.own_netframe(), options.work_thread_number()),
    m_timer_manager("client_timer"),
    m_thread_pool("client_thread_pool", 0,
                  options.has_callback_thread_number() ? options.callback_thread_number(): -1),
    m_channel_cache_size(options.has_channel_cache_size() ?
                         options.channel_cache_size() :
                         kDefaultChannelCacheSize),
    m_address_resolver(new ResolverFactory(
                NewPermanentClosure(this, &RpcClientImpl::OnChannelAddressChanged))),
    m_has_shutdown(false)
{
    if (options.own_netframe())
        CHECK_NOTNULL(netframe);
    memset(&m_memory_usage, 0, sizeof(m_memory_usage));
}

RpcClientImpl::RpcClientImpl(
    netframe::NetFrame* netframe,
    bool own_netframe,
    uint32_t cache_limit) :
    HttpClient(netframe, own_netframe),
    m_timer_manager("client_timer"),
    m_thread_pool("client_thread_pool"),
    m_channel_cache_size(cache_limit),
    m_address_resolver(new ResolverFactory(
                NewPermanentClosure(this, &RpcClientImpl::OnChannelAddressChanged))),
    m_has_shutdown(false)
{
    if (own_netframe)
        CHECK_NOTNULL(netframe);
}

RpcClientImpl::RpcClientImpl(int num_threads) :
    HttpClient(num_threads),
    m_timer_manager("client_timer"),
    m_thread_pool("client_thread_pool"),
    m_channel_cache_size(kDefaultChannelCacheSize),
    m_address_resolver(new ResolverFactory(
                NewPermanentClosure(this, &RpcClientImpl::OnChannelAddressChanged))),
    m_has_shutdown(false)
{
}

RpcClientImpl::~RpcClientImpl()
{
    delete m_address_resolver;
    m_address_resolver = NULL;
    if (!AtomicExchange(&m_has_shutdown, true)) {
        ShutdownAllChannels(false);
    }
    m_timer_manager.Stop();
}

static RpcClientOptions GetDefaultInstanceOptions()
{
    RpcClientOptions options;
    options.set_work_thread_number(::FLAGS_poppy_client_work_thread_number);
    options.set_callback_thread_number(::FLAGS_poppy_client_callback_thread_number);
    options.set_channel_cache_size(::FLAGS_poppy_client_channel_cache_size);
    return options;
}

stdext::shared_ptr<RpcClientImpl> RpcClientImpl::DefaultInstance()
{
    static stdext::shared_ptr<RpcClientImpl> client(
        new RpcClientImpl(GetDefaultInstanceOptions(), NULL));
    return client;
}

void RpcClientImpl::Shutdown(bool wait_all_pending_request_finish)
{
    {
        Mutex::Locker locker(m_channel_mutex);
        delete m_address_resolver;
        m_address_resolver = NULL;
    }
    if (AtomicExchange(&m_has_shutdown, true)) {
        VLOG(3) << "The RpcClientImpl is shutdown already.";
        return;
    }

    MutexLocker locker(m_channel_mutex);
    ShutdownAllChannels(wait_all_pending_request_finish);
}

void RpcClientImpl::ShutdownAllChannels(bool wait_all_pending_request_finish)
{
    RpcChannelSet::iterator it;
    for (it = m_channels.begin(); it != m_channels.end(); ++it) {
        (*it)->Shutdown(wait_all_pending_request_finish);
    }

    for (it = m_channel_cache.begin(); it != m_channel_cache.end(); ++it) {
        (*it)->Shutdown(false);
    }
}

TimerManager* RpcClientImpl::mutable_timer_manager()
{
    return &m_timer_manager;
}

namespace {

uint64_t String2Hash64(const std::string& str)
{
    return CityHash64(str.data(), str.length());
}

} // namespace

void RpcClientImpl::OnChannelAddressChanged(std::string channel_name,
                                            std::vector<std::string> addresses)
{
    RpcChannelImplPtr channel = GetChannelImpl(channel_name);
    // Data changed. Set address list to new value.
    if (channel != NULL) {
        channel->OnAddressesChanged(addresses);
    }
}

static uint64_t GetChannelHash(const std::string& name,
                               const RpcChannelOptions& options)
{
    // Calculate hash value based on service name or
    // by uuid if we don't want to use channel cache
    uint64_t hash_value;
    if (options.use_channel_cache()) {
        hash_value = String2Hash64(name);
    } else {
        Uuid uuid;
        uuid.Generate();
        hash_value = String2Hash64(uuid.ToString());
    }
    return hash_value;
}

RpcChannelImplPtr RpcClientImpl::GetChannelImpl(const std::string& name,
                                                common::CredentialGenerator* credential_generator,
                                                const RpcChannelOptions& options)
{
    CHECK_EQ(AtomicGet(&m_has_shutdown), false)
        << "The RpcClientImpl is shutdown already.";

    uint64_t hash_value = GetChannelHash(name, options);

    Mutex::Locker locker(m_channel_mutex);

    CHECK_EQ(AtomicGet(&m_has_shutdown), false)
        << "The RpcClientImpl is shutdown already.";

    RpcChannelImplPtr impl;

    // Fetch from cache or create a new one.
    RpcChannelMap::iterator it_map = m_channel_map.find(hash_value);
    if (it_map != m_channel_map.end()) {
        impl = (*it_map).second;
        // Remove from cache cache if exists.
        m_channel_cache.erase(impl.get());
    } else {
        std::vector<std::string> servers;
        int retry_times = 1;
        for (; retry_times <= ::FLAGS_poppy_resolve_address_retry_times; ++retry_times) {
            if (m_address_resolver->Resolve(name, &servers))
                break;
        }
        CHECK_LE(retry_times, ::FLAGS_poppy_resolve_address_retry_times)
            << "Failed to resolve ip addresses of the server: "
            << name
            << " for " << ::FLAGS_poppy_resolve_address_retry_times << " times";
        impl.reset(new RpcChannelImpl(shared_from_this(), name, hash_value, servers,
                                      credential_generator, options));
        // After we create a channel_impl, make connection immediately
        // as we use shared_from_this() in MakeConnectionWithDelay,
        // so we can't call it in channel_impl's constructor directly,
        impl->AddAllConnections();
        impl->MakeConnectionWithDelay(0);
        CHECK(m_channel_map.insert(std::make_pair(hash_value, impl)).second)
            << "A same hash key exists: " << hash_value;
    }

    // Insert into channel set if not exist.
    m_channels.insert(impl.get());

    return impl;
}

void RpcClientImpl::ReleaseChannelImpl()
{
    Mutex::Locker locker(m_channel_mutex);

    // Maintain the in using list and cache list.
    RpcChannelSet::iterator it;
    for (it = m_channels.begin(); it != m_channels.end();) {
        RpcChannelImplPtr& impl = m_channel_map.find((*it)->GetHash())->second;
        if (impl.unique()) {
            m_channel_cache.insert(*it);
            m_channels.erase(it++);
        } else {
            ++it;
        }
    }

    uint32_t size = m_channel_cache.size();
    for (it = m_channel_cache.begin();
         size > m_channel_cache_size && it != m_channel_cache.end();) {
        if ((*it)->GetConnectStatus() == RpcConnection::Disconnected) {
            (*it)->Shutdown(false);
            m_channel_map.erase(String2Hash64((*it)->GetName()));
            m_channel_cache.erase(it++);
            --size;
        } else {
            ++it;
        }
    }
}

// Connect to a server.
bool RpcClientImpl::AsyncConnect(const std::string& address,
                                 HttpHandler* handler,
                                 const netframe::NetFrame::EndPointOptions& options,
                                 HttpConnection** http_connection)
{
    VLOG(3) << "Connecting to " << address;

    SocketAddressInet addr(address);
    *http_connection = new HttpClientConnection(this, handler);
    int64_t socket_id = mutable_net_frame()->AsyncConnect(
            addr,
            *http_connection,
            HttpBase::kMaxMessageSize,
            options);
    if (socket_id < 0) {
        LOG(ERROR) << "Fail to connect to: " << address;
        delete *http_connection;
        *http_connection = NULL;
        return false;
    }
    return true;
}

void RpcClientImpl::GetMemoryUsage(MemoryUsage* memory_usage) const
{
    AtomicSet(&m_memory_usage.send_buffer_size,
              mutable_net_frame()->GetCurrentSendBufferedLength());
    AtomicSet(&m_memory_usage.receive_buffer_size,
              mutable_net_frame()->GetCurrentReceiveBufferedLength());
    *memory_usage = m_memory_usage;
}

void RpcClientImpl::IncreaseRequestMemorySize(size_t size)
{
    AtomicAdd(&m_memory_usage.queued_size, size);
}

void RpcClientImpl::DecreaseRequestMemorySize(size_t size)
{
    AtomicSub(&m_memory_usage.queued_size, size);
}

} // namespace poppy
