// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Dongping HUANG <dphuang@tencent.com>

#include "poppy/rpc_request_queue.h"

#include <algorithm>
#include <limits>
#include <list>
#include <sstream>
#include <utility>

#include "common/base/static_assert.h"
#include "common/system/concurrency/this_thread.h"
#include "common/system/time/timestamp.h"

#include "poppy/rpc_client_impl.h"
#include "poppy/rpc_controller.h"
// includes from thirdparty
#include "thirdparty/glog/logging.h"

namespace poppy {

static const int64_t kTimeInterval = 32;
static const int64_t kMaxTimeStamp = std::numeric_limits<int64_t>::max() / 2;

RpcWorkload::RpcWorkload() :
    m_request_count(0),
    m_response_count(0),
    m_canceled_count(0),
    m_timeout_count(0),
    m_takeaway_count(0),
    m_pending_count(0),
    m_last_use_time(GetTimeStampInMs()),
    m_next(NULL)
{
}

uint64_t RpcWorkload::AddRequest(uint64_t count)
{
    Mutex::Locker locker(m_mutex);
    m_request_count += count;
    m_pending_count += count;

    if (m_next != NULL) {
        m_next->AddRequest(count);
    }

    return m_pending_count;
}

uint64_t RpcWorkload::ConfirmRequest(RequestConfirmType type, uint64_t count)
{
    Mutex::Locker locker(m_mutex);
    switch (type) {
    case Response:
        m_response_count += count;
        break;
    case Canceled:
        m_canceled_count += count;
        break;
    case Timeout:
        m_timeout_count += count;
        break;
    case TakeAway:
        m_takeaway_count += count;
    }
    m_pending_count -= count;

    if (m_next != NULL) {
        m_next->ConfirmRequest(type, count);
    }

    return m_pending_count;
}

uint64_t RpcWorkload::GetPendingCount() const
{
    Mutex::Locker locker(m_mutex);
    return m_pending_count;
}

void RpcWorkload::SetNext(RpcWorkload *next)
{
    m_next = next;
}

RpcWorkload* RpcWorkload::GetNext()
{
    return m_next;
}

void RpcWorkload::SetLastUseTime()
{
    int64_t timestamp = GetTimeStampInMs();
    SetLastUseTime(timestamp);
}

void RpcWorkload::SetLastUseTime(int64_t timestamp)
{
    m_last_use_time = std::max(m_last_use_time, timestamp);
    if (m_next != NULL) {
        m_next->SetLastUseTime(timestamp);
    }
}

int64_t RpcWorkload::GetLastUseTime() const
{
    Mutex::Locker locker(m_mutex);
    return m_last_use_time;
}

std::string RpcWorkload::Dump() const
{
    Mutex::Locker locker(m_mutex);
    std::stringstream ss;
    ss << "Total request count: " << m_request_count << "\n"
       << "\tresponse count: " << m_response_count << "\n"
       << "\tcanceled count: " << m_canceled_count << "\n"
       << "\ttime out count: " << m_timeout_count << "\n"
       << "\ttakeaway count: " << m_takeaway_count << "\n"
       << "\tpending count: " << m_pending_count << "\n";
    return ss.str();
}

RpcRequestQueue::RpcRequestQueue(RpcClientImpl* rpc_client_impl, TimerManager *timer_manager) :
    m_rpc_client_impl(rpc_client_impl),
    m_timer_manager(timer_manager),
    m_timer_id(0),
    m_timeout(0),
    m_default_error(RPC_ERROR_REQUEST_TIMEOUT)
{
}

// Response callbacks for all pending requests would be called.
RpcRequestQueue::~RpcRequestQueue()
{
    Shutdown();
}

void RpcRequestQueue::Shutdown()
{
    CancelAll(RPC_ERROR_CHANNEL_SHUTDOWN);

    while (true) {
        {
            Mutex::Locker locker(m_mutex);
            if (m_timer_id == 0) {
                break;
            } else if (m_timer_manager->AsyncRemoveTimer(m_timer_id)) {
                m_timer_id = 0;
                break;
            }
        }
        ThisThread::Sleep(1);
    }
}

// Add a new request (all information about a request) to the queue.
// It assumes the sequence id of rpc request has been allocated and it's
// caller's responsibility to guarantee no duplicated sequence id.
void RpcRequestQueue::Add(RpcRequest* rpc_request, int64_t expected_timeout, bool count)
{
    Mutex::Locker locker(m_mutex);
    // Sanity check.
    CHECK_GE(rpc_request->sequence_id, 0);
    CHECK_GE(rpc_request->controller->Timeout(), 0);

    // Insert the request into the request queue.
    CHECK(m_requests.insert(
                std::make_pair(rpc_request->sequence_id, rpc_request)).second)
            << "Duplicated sequence id in request queue: "
            << rpc_request->sequence_id;

    Timeout timeout = { expected_timeout, rpc_request->sequence_id };
    m_timeout_queue.push(timeout);

    if (count) {
        m_workload.AddRequest(1);
        m_rpc_client_impl->IncreaseRequestMemorySize(rpc_request->MemoryUsage());
    }

    UpdateTimer();
}

// Remove the request from queue and return the that request.
// Return NULL if the sequence id doesn't exist.
RpcRequest* RpcRequestQueue::RemoveAndConfirm(
    const int64_t sequence_id,
    RpcWorkload::RequestConfirmType type)
{
    Mutex::Locker locker(m_mutex);
    RequestQueue::iterator it = m_requests.find(sequence_id);

    if (it == m_requests.end()) {
        return NULL;
    }

    // Remove item for request queue.
    RpcRequest* rpc_request = it->second;
    m_requests.erase(it);

    UpdateTimer();

    if (!rpc_request->IsBuiltin()) {
        m_workload.ConfirmRequest(type, 1);
        m_rpc_client_impl->DecreaseRequestMemorySize(rpc_request->MemoryUsage());
    }
    return rpc_request;
}

// Redispatch first undispatched request to specified connection.
RpcRequest* RpcRequestQueue::PopFirst()
{
    Mutex::Locker locker(m_mutex);
    if (m_requests.empty()) {
        return NULL;
    }

    RpcRequest* rpc_request = m_requests.begin()->second;
    m_requests.erase(m_requests.begin());

    UpdateTimer();
    m_workload.ConfirmRequest(RpcWorkload::TakeAway, 1);

    return rpc_request;
}

uint64_t RpcRequestQueue::CancelAll(RpcErrorCode error_code)
{
    Mutex::Locker locker(m_mutex);
    uint64_t count = 0;
    for (RequestQueue::const_iterator it = m_requests.begin();
         it != m_requests.end();
         ++it) {
        RpcRequest* rpc_request = it->second;

        rpc_request->controller->SetFailed(error_code);
        if (!rpc_request->IsBuiltin()) {
            ++count;
        }

        Cancel(rpc_request);
    }

    m_requests.clear();
    m_timeout_queue = TimeoutQueue();

    UpdateTimer();

    m_workload.ConfirmRequest(RpcWorkload::Canceled, count);

    return count;
}

uint64_t RpcRequestQueue::RemoveAll(std::list<RpcRequest*>* requests)
{
    Mutex::Locker locker(m_mutex);
    uint64_t count = 0;
    for (RequestQueue::const_iterator it = m_requests.begin();
         it != m_requests.end();
         ++it) {
        RpcRequest* rpc_request = it->second;

        if (rpc_request->IsBuiltin() ||
                rpc_request->controller->fail_immediately()) {
            rpc_request->controller->SetFailed(RPC_ERROR_CONNECTION_CLOSED);
            Cancel(rpc_request);
        } else  {
            requests->push_back(rpc_request);
            ++count;
        }
    }

    m_timeout_queue = TimeoutQueue();

    UpdateTimer();

    m_workload.ConfirmRequest(RpcWorkload::TakeAway, count);

    return count;
}

int64_t RpcRequestQueue::GetTimeout()
{
    // Mutex::Locker locker(m_mutex);
    // break if no timeouts or the first timeout doesn't happen.
    while (!m_timeout_queue.empty()) {
        int64_t sequence_id = m_timeout_queue.top().sequence_id;
        std::map<int64_t, RpcRequest*>::iterator iter =
            m_requests.find(sequence_id);

        if (iter == m_requests.end()) {
            // The request has already been processed. Response is back.
            // Just ignore it.
            m_timeout_queue.pop();
            continue;
        }

        int64_t time = m_timeout_queue.top().time;
        // round to kTimeInterval;
        int64_t rounded_time =
            (time + kTimeInterval - 1) / kTimeInterval * kTimeInterval;
        return rounded_time;
    }
    return kMaxTimeStamp;
}

uint64_t RpcRequestQueue::ProcessTimeoutRequest(int64_t time, RpcErrorCode error)
{
    int64_t timestamp = GetTimeStampInMs();
    std::list<RpcRequest*> timeout_requests;
    {
        Mutex::Locker locker(m_mutex);
        // break if no timeouts or the first timeout doesn't happen.
        while (!m_timeout_queue.empty() &&
               m_timeout_queue.top().time <= timestamp) {
            int64_t sequence_id = m_timeout_queue.top().sequence_id;
            std::map<int64_t, RpcRequest*>::iterator iter =
                m_requests.find(sequence_id);

            if (iter == m_requests.end()) {
                // The request has already been processed. Response is back.
                // Just ignore it.
                m_timeout_queue.pop();
                continue;
            }

            RpcRequest* rpc_request = iter->second;
            // Remove the request from request queue.
            m_requests.erase(iter);
            m_timeout_queue.pop();

            timeout_requests.push_back(rpc_request);
        }
    }

    uint64_t count = 0;
    for (std::list<RpcRequest*>::const_iterator i = timeout_requests.begin();
         i != timeout_requests.end();
         ++i) {
        RpcRequest* rpc_request = *i;

        rpc_request->controller->SetFailed(error);
        VLOG(4) << "Cancel request because timeout: "
            << rpc_request->sequence_id;
        if (!rpc_request->IsBuiltin()) {
            ++count;
        }
        Cancel(rpc_request);
    }

    m_workload.ConfirmRequest(RpcWorkload::Timeout, count);

    return count;
}

// Call this function when a rpc request is being canceled, because of timeout
// or connection being closed.
void RpcRequestQueue::Cancel(RpcRequest* rpc_request)
{
    CHECK(rpc_request->controller->Failed())
            << "The controller must be set as failed when being canceled.";
    rpc_request->done->Run(NULL);
    delete rpc_request;
}

bool RpcRequestQueue::IsEmpty() const
{
    Mutex::Locker locker(m_mutex);
    return m_requests.empty();
}

uint64_t RpcRequestQueue::GetPendingCount() const
{
    Mutex::Locker locker(m_mutex);
    return m_requests.size();
}

bool RpcRequestQueue::UpdateTimer()
{
    assert(m_mutex.IsLocked());

    int64_t new_time = GetTimeout();

    if (m_timeout == new_time) {
        return true;
    }

    if (m_timer_id != 0) {
        if (m_timer_manager->AsyncRemoveTimer(m_timer_id)) {
            m_timer_id = 0;
            m_timeout = 0;
        } else {
            return false;
        }
    }

    if (new_time == kMaxTimeStamp) {
        m_timer_id = 0;
        m_timeout = 0;
        return true;
    }

    int64_t time_out =
        std::max<int64_t>(new_time - GetTimeStampInMs(), 0);
    TimerManager::CallbackFunction func = Bind(
        &RpcRequestQueue::OnTimer, this);
    m_timer_id = m_timer_manager->AddOneshotTimer(time_out, func);
    m_timeout = new_time;

    return true;
}

void RpcRequestQueue::OnTimer(uint64_t timer_id)
{
    int64_t timestamp = GetTimeStampInMs();
    ProcessTimeoutRequest(timestamp, m_default_error);

    Mutex::Locker locker(m_mutex);
    if (timer_id != m_timer_id) {
        return;
    }

    m_timer_id = 0;
    m_timeout = 0;

    UpdateTimer();
}

bool RpcRequestQueue::StopTimer()
{
    assert(m_mutex.IsLocked());

    if (m_timer_id != 0 && m_timer_manager->AsyncRemoveTimer(m_timer_id)) {
        m_timer_id = 0;
    }

    return m_timer_id == 0;
}

RpcWorkload* RpcRequestQueue::GetWorkload()
{
    return &m_workload;
}

void RpcRequestQueue::SetDefaultErrorCode(RpcErrorCode err_code)
{
    m_default_error = err_code;
}

} // namespace poppy

