// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
// Dongping HUANG (dphuang@tencent.com)
//
// Define rpc request and rpc request queue.

#ifndef POPPY_RPC_REQUEST_QUEUE_H
#define POPPY_RPC_REQUEST_QUEUE_H

#include <functional>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "common/base/stdint.h"
#include "common/system/timer/timer_manager.h"

#include "poppy/rpc_request.h"

namespace poppy {

class RpcClientImpl;

//
// RpcWorkload is a internal tool used to manage the work load of channel
// and connections.
// It count the total request, and count how we handed them. The hande
// type includes 'responsed by server', 'canceled by user', 'take away
// from here', and 'timed out'.
//
class RpcWorkload
{
public:
    // How a request sent to poppy be confirmed.
    enum RequestConfirmType {
        Response = 0,   // normal case
        Canceled,       // connection is closed
        TakeAway,       // those take away be user
        Timeout         // response time out
    };

public:
    RpcWorkload();

    uint64_t AddRequest(uint64_t count = 1);
    uint64_t ConfirmRequest(RequestConfirmType type, uint64_t count = 1);
    uint64_t GetPendingCount() const;
    void SetNext(RpcWorkload* next);
    void SetLastUseTime(int64_t timestamp);
    void SetLastUseTime();
    int64_t GetLastUseTime() const;
    RpcWorkload* GetNext();
    std::string Dump() const;

private:
    mutable Mutex m_mutex;
    uint64_t m_request_count;   // request sent to this connection
    uint64_t m_response_count;  // response get from this connection
    uint64_t m_canceled_count;  // canceled request count
    uint64_t m_timeout_count;   // request count canceled because of timeout
    uint64_t m_takeaway_count;  // those taken away from this queue
    uint64_t m_pending_count;   //
    int64_t m_last_use_time;
    RpcWorkload *m_next;
};

// A simple queue for pending rpc requests.
class RpcRequestQueue
{
public:
    RpcRequestQueue(RpcClientImpl* rpc_client_impl, TimerManager* timer_manager);
    ~RpcRequestQueue();

    void Shutdown();

    // Add a new request (all information about a request) to the queue.
    // It assumes the sequence id of rpc request has been allocated
    void Add(RpcRequest* rpc_request, int64_t expected_timeout, bool count = true);

    // Remove the request from queue and return the that request.
    // Return NULL if the sequence id doesn't exist.
    RpcRequest* RemoveAndConfirm(int64_t sequence_id,
                                        RpcWorkload::RequestConfirmType type);

    // Pop first request and return it. Return NULL if no request anymore.
    RpcRequest* PopFirst();

    // Cancel all the requests in the queue with error_code.
    // Return the totoal number of canceled requests.
    uint64_t CancelAll(RpcErrorCode error_code);

    // Release all pending requests for a connection, and cancel them if
    // "cancel_requests" is true. If "cancel_requests" is false, the request
    // will not be canceled and just flaged as "undispatched" by set connection
    // id to -1.
    // It's called when a connection is closed.
    uint64_t RemoveAll(std::list<RpcRequest*>* requests);

    uint64_t ProcessTimeoutRequest(int64_t time, RpcErrorCode error);

    bool IsEmpty() const;

    uint64_t GetPendingCount() const;

    bool StopTimer();

    bool UpdateTimer();

    void OnTimer(uint64_t timer_id);

    RpcWorkload* GetWorkload();

    void SetDefaultErrorCode(RpcErrorCode err_code);

private:
    void Cancel(RpcRequest* rpc_request);

    int64_t GetTimeout();

private:
    // Timeout struct, record timeout information of a request.
    struct Timeout
    {
        int64_t time;
        int64_t sequence_id;
        bool operator > (const Timeout& rhs) const
        {
            if (time > rhs.time) {
                return true;
            } else if (time < rhs.time) {
                return false;
            }

            return sequence_id > rhs.sequence_id;
        }
    };

    mutable Mutex m_mutex;

    // Pending requests queue. <sequence_id, RpcRequest> pairs.
    typedef std::map<int64_t, RpcRequest*> RequestQueue;
    RequestQueue m_requests;
    RpcClientImpl* m_rpc_client_impl;
    TimerManager* m_timer_manager;
    uint64_t m_timer_id;
    // Time out @
    int64_t m_timeout;
    // Timeout queue.
    typedef std::priority_queue < Timeout, std::vector<Timeout>,
            std::greater<Timeout> > TimeoutQueue;
    TimeoutQueue m_timeout_queue;

    RpcWorkload m_workload;
    RpcErrorCode m_default_error;
};

} // namespace poppy

#endif // POPPY_RPC_REQUEST_QUEUE_H
