// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef POPPY_STATS_REGISTRY_H
#define POPPY_STATS_REGISTRY_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "common/base/timed_stats.h"
#include "common/base/uncopyable.h"
#include "common/system/concurrency/mutex.h"
#include "common/system/concurrency/rwlock.h"
#include "thirdparty/ctemplate/template.h"

namespace poppy {

// using namespace common;
struct RpcFailureStatsInfo {
    CountingStats<int64_t> total_failure_stats;
    mutable RWLock rwlock;
    // key is error code defined in rpc_error_code_info.proto
    std::map<int, CountingStats<int64_t>* > failure_detail_stats;

    ~RpcFailureStatsInfo();
};

struct RpcStatsInfo {
    CountingStats<int64_t> request_stats;
    CountingStats<int64_t> success_stats;
    RpcFailureStatsInfo failure_stats;
    MeanStats<int64_t> latency_stats;
};

struct MemoryStatsInfo {
    MeanStats<size_t> rpc_memory_stats;
    MeanStats<size_t> net_send_memory_stats;
    MeanStats<size_t> net_receive_memory_stats;
};

class StatsRegistry
{
public:
    StatsRegistry();
    ~StatsRegistry();

    bool RegisterMethod(const std::string& method_name);

    void AddGlobalCounter(bool success, int error_code = 0);
    // Call AddMethodCounterForUser will also add global counter
    void AddMethodCounterForUser(const std::string& user,
            const std::string& method_name,
            bool success,
            int error_code = 0);
    void AddMethodLatencyForUser(const std::string& user,
            const std::string& method_name,
            int64_t latency, int64_t time);

    void UpdateRpcMemorySize(size_t memory_size, int64_t time);

    void AddNetFrameSendMemorySize(size_t memory_size, int64_t time);

    void AddNetFrameReceiveMemorySize(size_t memory_size, int64_t time);

    void Dump(ctemplate::TemplateDictionary* dict);

    const RpcStatsInfo& GetGlobalRpcStats() const
    {
        return m_global_rpc_stats;
    }

private:
    RpcStatsInfo* GetRpcStatsInfo(const std::string& user,
                                  const std::string& method_name);

    void AddRpcStatsFailureCount(RpcStatsInfo* stats_info,
                                 int error_code);

private:
    mutable RWLock m_rwlock;
    typedef std::map<std::string, RpcStatsInfo*> InternalMap;
    std::map<std::string, InternalMap*> m_stats_map;
    // Store methods that need stats
    std::set<std::string> m_methods;
    RpcStatsInfo m_global_rpc_stats;
    MemoryStatsInfo m_memory_stats;

    // Dump stats unit
    void DumpRpcStatsUnit(ctemplate::TemplateDictionary* dict,
            const std::string& section_name,
            RpcStatsInfo* stats);
    void DumpMemoryStatsUnit(ctemplate::TemplateDictionary* dict,
            const std::string& section_name,
            MemoryStatsInfo* stats);

    DECLARE_UNCOPYABLE(StatsRegistry);
};

} // namespace poppy

#endif // POPPY_STATS_REGISTRY_H
