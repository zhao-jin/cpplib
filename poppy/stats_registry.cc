// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/stats_registry.h"
#include <utility>
#include "common/base/string/string_number.h"
#include "poppy/rpc_error_code.h"
#include "thirdparty/glog/logging.h"

namespace poppy {

// using namespace common;

RpcFailureStatsInfo::~RpcFailureStatsInfo()
{
    std::map<int, CountingStats<int64_t>* >::iterator iter;
    for (iter = failure_detail_stats.begin();
         iter != failure_detail_stats.end();
         ++iter) {
        delete iter->second;
    }
}

StatsRegistry::StatsRegistry()
{
    // Register global stats
    m_global_rpc_stats.request_stats.SetDescription("request count", "times");
    m_global_rpc_stats.success_stats.SetDescription("success count", "times");
    m_global_rpc_stats.failure_stats.total_failure_stats.SetDescription("failure count", "times");
    m_global_rpc_stats.latency_stats.SetDescription("latency", "ms");
    m_memory_stats.rpc_memory_stats.SetDescription("rpc queue memory", "B");
    m_memory_stats.net_send_memory_stats.SetDescription("net send memory", "KB");
    m_memory_stats.net_receive_memory_stats.SetDescription("net recv memory", "KB");
}

StatsRegistry::~StatsRegistry()
{
    std::map<std::string, InternalMap*>::iterator i1 = m_stats_map.begin();
    for (; i1 != m_stats_map.end(); ++i1) {
        InternalMap::iterator i2 = i1->second->begin();
        for (; i2 != i1->second->end(); ++i2) {
            delete i2->second;
        }
        delete i1->second;
    }
}

RpcStatsInfo* StatsRegistry::GetRpcStatsInfo(
    const std::string& user,
    const std::string& method_name)
{
    InternalMap* map = NULL;
    RpcStatsInfo* stats = NULL;
    std::map<std::string, InternalMap*>::iterator iter_user;
    InternalMap::iterator iter_method;
    bool add_flag = false;
    {
        RWLock::ReaderLocker reader_locker(m_rwlock);
        CHECK(m_methods.find(method_name) != m_methods.end())
            << method_name << " is invalid method name";

        iter_user = m_stats_map.find(user);
        add_flag = (iter_user == m_stats_map.end());
        if (!add_flag) {
            map = iter_user->second;
        }
    }
    if (add_flag) {
        RWLock::WriterLocker writer_lock(m_rwlock);
        iter_user = m_stats_map.find(user);
        if (iter_user == m_stats_map.end()) {
            map = new InternalMap();
            m_stats_map.insert(std::pair<std::string, InternalMap*>(user, map));
        } else {
            map = iter_user->second;
        }
    }

    {
        RWLock::ReaderLocker reader_locker(m_rwlock);
        iter_method = map->find(method_name);
        add_flag = (iter_method == map->end());
        if (!add_flag) {
            stats = iter_method->second;
        }
    }
    if (add_flag) {
        RWLock::WriterLocker writer_lock(m_rwlock);
        iter_method = map->find(method_name);
        if (iter_method == map->end()) {
            stats = new RpcStatsInfo;
            stats->request_stats.SetDescription("request count", "times");
            stats->success_stats.SetDescription("success count", "times");
            stats->failure_stats.total_failure_stats.SetDescription("failure count", "times");
            stats->latency_stats.SetDescription("latency", "ms");
            map->insert(std::pair<std::string, RpcStatsInfo*>(method_name, stats));
        } else {
            stats = iter_method->second;
        }
    }

    return stats;
}

bool StatsRegistry::RegisterMethod(const std::string& method_name)
{
    RWLock::WriterLocker locker(m_rwlock);
    return m_methods.insert(method_name).second;
}

void StatsRegistry::AddGlobalCounter(bool success, int error_code)
{
    if (success) {
        m_global_rpc_stats.success_stats.Increase();
    } else {
        AddRpcStatsFailureCount(&m_global_rpc_stats, error_code);
    }
    m_global_rpc_stats.request_stats.Increase();
}

void StatsRegistry::AddRpcStatsFailureCount(RpcStatsInfo* stats_info,
                                            int error_code)
{
    stats_info->failure_stats.total_failure_stats.Increase();
    std::map<int, CountingStats<int64_t>* >::iterator iter;
    bool add_flag = false;
    {
        RWLock::ReaderLocker reader_locker(stats_info->failure_stats.rwlock);
        iter = stats_info->failure_stats.failure_detail_stats.find(error_code);
        add_flag = (iter == stats_info->failure_stats.failure_detail_stats.end());
    }
    if (add_flag) {
        RWLock::WriterLocker writer_lock(stats_info->failure_stats.rwlock);
        iter = stats_info->failure_stats.failure_detail_stats.find(error_code);
        if (iter == stats_info->failure_stats.failure_detail_stats.end()) {
            CountingStats<int64_t>* failure_stats = new CountingStats<int64_t>();
            failure_stats->SetDescription(RpcErrorCodeToString(error_code), "times");
            failure_stats->Increase();
            stats_info->failure_stats.failure_detail_stats[error_code] = failure_stats;
        }
    } else {
        (*iter).second->Increase();
    }
}

void StatsRegistry::AddMethodCounterForUser(const std::string& user,
        const std::string& method_name,
        bool success,
        int error_code)
{
    RpcStatsInfo* stats = GetRpcStatsInfo(user, method_name);
    if (success) {
        stats->success_stats.Increase();
        m_global_rpc_stats.success_stats.Increase();
    } else {
        AddRpcStatsFailureCount(stats, error_code);
        AddRpcStatsFailureCount(&m_global_rpc_stats, error_code);
    }

    stats->request_stats.Increase();
    m_global_rpc_stats.request_stats.Increase();
}

void StatsRegistry::AddMethodLatencyForUser(const std::string& user,
        const std::string& method_name,
        int64_t latency, int64_t time)
{
    RpcStatsInfo* stats = GetRpcStatsInfo(user, method_name);
    stats->latency_stats.Add(latency, time);
    m_global_rpc_stats.latency_stats.Add(latency, time);
}

void StatsRegistry::UpdateRpcMemorySize(size_t memory_size, int64_t time)
{
    m_memory_stats.rpc_memory_stats.Add(memory_size, time);
}

void StatsRegistry::AddNetFrameSendMemorySize(size_t memory_size, int64_t time)
{
    m_memory_stats.net_send_memory_stats.Add(memory_size, time);
}

void StatsRegistry::AddNetFrameReceiveMemorySize(size_t memory_size, int64_t time)
{
    m_memory_stats.net_receive_memory_stats.Add(memory_size, time);
}

void StatsRegistry::DumpMemoryStatsUnit(ctemplate::TemplateDictionary* dict,
        const std::string& section_name,
        MemoryStatsInfo* stats)
{
    MeanStatsResult<size_t> rpc_memory_stats_result;
    MeanStatsResult<size_t> net_send_memory_stats_result;
    MeanStatsResult<size_t> net_receive_memory_stats_result;
    stats->rpc_memory_stats.GetStatsResult(&rpc_memory_stats_result);
    stats->net_send_memory_stats.GetStatsResult(&net_send_memory_stats_result);
    stats->net_receive_memory_stats.GetStatsResult(&net_receive_memory_stats_result);

    dict->SetValue("RPC_MEMORY_LAST_SECOND_MIN",
                   FormatMeasure(rpc_memory_stats_result.last_second.min));
    dict->SetValue("RPC_MEMORY_LAST_SECOND_MAX",
                   FormatMeasure(rpc_memory_stats_result.last_second.max));
    dict->SetValue("RPC_MEMORY_LAST_SECOND_AVERAGE",
                   FormatMeasure(rpc_memory_stats_result.last_second.average));
    dict->SetValue("RPC_MEMORY_LAST_MINUTE_MIN",
                   FormatMeasure(rpc_memory_stats_result.last_minute.min));
    dict->SetValue("RPC_MEMORY_LAST_MINUTE_MAX",
                   FormatMeasure(rpc_memory_stats_result.last_minute.max));
    dict->SetValue("RPC_MEMORY_LAST_MINUTE_AVERAGE",
                   FormatMeasure(rpc_memory_stats_result.last_minute.average));
    dict->SetValue("RPC_MEMORY_LAST_HOUR_MIN",
                   FormatMeasure(rpc_memory_stats_result.last_hour.min));
    dict->SetValue("RPC_MEMORY_LAST_HOUR_MAX",
                   FormatMeasure(rpc_memory_stats_result.last_hour.max));
    dict->SetValue("RPC_MEMORY_LAST_HOUR_AVERAGE",
                   FormatMeasure(rpc_memory_stats_result.last_hour.average));
    dict->SetValue("RPC_MEMORY_TOTAL_MIN",
                   FormatMeasure(rpc_memory_stats_result.total.min));
    dict->SetValue("RPC_MEMORY_TOTAL_MAX",
                   FormatMeasure(rpc_memory_stats_result.total.max));
    dict->SetValue("RPC_MEMORY_TOTAL_AVERAGE",
                   FormatMeasure(rpc_memory_stats_result.total.average));

    dict->SetValue("NET_SEND_MEMORY_LAST_SECOND_MIN",
                   FormatMeasure(net_send_memory_stats_result.last_second.min));
    dict->SetValue("NET_SEND_MEMORY_LAST_SECOND_MAX",
                   FormatMeasure(net_send_memory_stats_result.last_second.max));
    dict->SetValue("NET_SEND_MEMORY_LAST_SECOND_AVERAGE",
                   FormatMeasure(net_send_memory_stats_result.last_second.average));
    dict->SetValue("NET_SEND_MEMORY_LAST_MINUTE_MIN",
                   FormatMeasure(net_send_memory_stats_result.last_minute.min));
    dict->SetValue("NET_SEND_MEMORY_LAST_MINUTE_MAX",
                   FormatMeasure(net_send_memory_stats_result.last_minute.max));
    dict->SetValue("NET_SEND_MEMORY_LAST_MINUTE_AVERAGE",
                   FormatMeasure(net_send_memory_stats_result.last_minute.average));
    dict->SetValue("NET_SEND_MEMORY_LAST_HOUR_MIN",
                   FormatMeasure(net_send_memory_stats_result.last_hour.min));
    dict->SetValue("NET_SEND_MEMORY_LAST_HOUR_MAX",
                   FormatMeasure(net_send_memory_stats_result.last_hour.max));
    dict->SetValue("NET_SEND_MEMORY_LAST_HOUR_AVERAGE",
                   FormatMeasure(net_send_memory_stats_result.last_hour.average));
    dict->SetValue("NET_SEND_MEMORY_TOTAL_MIN",
                   FormatMeasure(net_send_memory_stats_result.total.min));
    dict->SetValue("NET_SEND_MEMORY_TOTAL_MAX",
                   FormatMeasure(net_send_memory_stats_result.total.max));
    dict->SetValue("NET_SEND_MEMORY_TOTAL_AVERAGE",
                   FormatMeasure(net_send_memory_stats_result.total.average));

    dict->SetValue("NET_RECV_MEMORY_LAST_SECOND_MIN",
                   FormatMeasure(net_receive_memory_stats_result.last_second.min));
    dict->SetValue("NET_RECV_MEMORY_LAST_SECOND_MAX",
                   FormatMeasure(net_receive_memory_stats_result.last_second.max));
    dict->SetValue("NET_RECV_MEMORY_LAST_SECOND_AVERAGE",
                   FormatMeasure(net_receive_memory_stats_result.last_second.average));
    dict->SetValue("NET_RECV_MEMORY_LAST_MINUTE_MIN",
                   FormatMeasure(net_receive_memory_stats_result.last_minute.min));
    dict->SetValue("NET_RECV_MEMORY_LAST_MINUTE_MAX",
                   FormatMeasure(net_receive_memory_stats_result.last_minute.max));
    dict->SetValue("NET_RECV_MEMORY_LAST_MINUTE_AVERAGE",
                   FormatMeasure(net_receive_memory_stats_result.last_minute.average));
    dict->SetValue("NET_RECV_MEMORY_LAST_HOUR_MIN",
                   FormatMeasure(net_receive_memory_stats_result.last_hour.min));
    dict->SetValue("NET_RECV_MEMORY_LAST_HOUR_MAX",
                   FormatMeasure(net_receive_memory_stats_result.last_hour.max));
    dict->SetValue("NET_RECV_MEMORY_LAST_HOUR_AVERAGE",
                   FormatMeasure(net_receive_memory_stats_result.last_hour.average));
    dict->SetValue("NET_RECV_MEMORY_TOTAL_MIN",
                   FormatMeasure(net_receive_memory_stats_result.total.min));
    dict->SetValue("NET_RECV_MEMORY_TOTAL_MAX",
                   FormatMeasure(net_receive_memory_stats_result.total.max));
    dict->SetValue("NET_RECV_MEMORY_TOTAL_AVERAGE",
                   FormatMeasure(net_receive_memory_stats_result.total.average));
}

void StatsRegistry::DumpRpcStatsUnit(ctemplate::TemplateDictionary* dict,
        const std::string& section_name,
        RpcStatsInfo* stats)
{
    CountingStatsResult<int64_t> count_stats_result;
    MeanStatsResult<int64_t> latency_stats_result;

    typedef std::pair<std::string, CountingStats<int64_t>*> PAIR;
    std::vector<PAIR> v;
    v.push_back(PAIR("request count", &stats->request_stats));
    v.push_back(PAIR("success count", &stats->success_stats));
    v.push_back(PAIR("failure count", &stats->failure_stats.total_failure_stats));

    {
        RWLock::WriterLocker locker(stats->failure_stats.rwlock);
        std::map<int, CountingStats<int64_t>* >::iterator iter;
        for (iter = stats->failure_stats.failure_detail_stats.begin();
             iter != stats->failure_stats.failure_detail_stats.end();
             ++iter) {
            v.push_back(PAIR(RpcErrorCodeToString(iter->first),
                             iter->second));
        }
    }

    for (unsigned int i = 0; i < v.size(); ++i) {
        v[i].second->GetStatsResult(&count_stats_result);
        ctemplate::TemplateDictionary* nest_dict =
            dict->AddSectionDictionary(section_name);
        nest_dict->SetValue("STATS_VARIABLE_NAME", v[i].first);
        nest_dict->SetIntValue("LAST_SECOND_COUNT", count_stats_result.last_second_count);
        nest_dict->SetIntValue("LAST_MINUTE_COUNT", count_stats_result.last_minute_count);
        nest_dict->SetIntValue("LAST_HOUR_COUNT", count_stats_result.last_hour_count);
        nest_dict->SetIntValue("TOTAL_COUNT", count_stats_result.total_count);
    }
    stats->latency_stats.GetStatsResult(&latency_stats_result);

    dict->SetIntValue("LAST_SECOND_MIN", latency_stats_result.last_second.min);
    dict->SetIntValue("LAST_SECOND_MAX", latency_stats_result.last_second.max);
    dict->SetIntValue("LAST_SECOND_AVERAGE", latency_stats_result.last_second.average);
    dict->SetIntValue("LAST_MINUTE_MIN", latency_stats_result.last_minute.min);
    dict->SetIntValue("LAST_MINUTE_MAX", latency_stats_result.last_minute.max);
    dict->SetIntValue("LAST_MINUTE_AVERAGE", latency_stats_result.last_minute.average);
    dict->SetIntValue("LAST_HOUR_MIN", latency_stats_result.last_hour.min);
    dict->SetIntValue("LAST_HOUR_MAX", latency_stats_result.last_hour.max);
    dict->SetIntValue("LAST_HOUR_AVERAGE", latency_stats_result.last_hour.average);
    dict->SetIntValue("TOTAL_MIN", latency_stats_result.total.min);
    dict->SetIntValue("TOTAL_MAX", latency_stats_result.total.max);
    dict->SetIntValue("TOTAL_AVERAGE", latency_stats_result.total.average);
}

void StatsRegistry::Dump(ctemplate::TemplateDictionary* dict)
{
    // Global status
    DumpRpcStatsUnit(dict, "GLOBAL_REQUEST_COUNT_ROW", &m_global_rpc_stats);
    DumpMemoryStatsUnit(dict, "GLOBAL_REQUEST_COUNT_ROW", &m_memory_stats);

    RWLock::WriterLocker locker(m_rwlock);

    // User method status
    std::map<std::string, InternalMap*>::iterator i1 = m_stats_map.begin();
    for (; i1 != m_stats_map.end(); ++i1) {
        ctemplate::TemplateDictionary* user_dict =
            dict->AddSectionDictionary("USER_ROW");
        user_dict->SetValue("USER_NAME",
                            (i1->first.empty() ? "anonymous" : i1->first));
        InternalMap::iterator i2 = i1->second->begin();
        for (; i2 != i1->second->end(); ++i2) {
            ctemplate::TemplateDictionary* method_dict =
                user_dict->AddSectionDictionary("METHOD_ROW");
            method_dict->SetValue("METHOD_FULL_NAME", i2->first);
            DumpRpcStatsUnit(method_dict, "REQUEST_COUNT_ROW", i2->second);
        }
    }
}

} // namespace poppy
