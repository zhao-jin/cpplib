// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu>

#ifndef POPPY_ADDRESS_RESOLVER_TNS_JOB_RESOLVER_H
#define POPPY_ADDRESS_RESOLVER_TNS_JOB_RESOLVER_H
#pragma once

#include <map>
#include <string>
#include <vector>
#include "common/system/concurrency/mutex.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "poppy/address_resolver/address_resolver.h"

namespace poppy {

class TnsJobAddressResolver : public AddressResolver
{
public:
    TnsJobAddressResolver(common::ZookeeperClient* client, AddressChangedCallback* callback) :
        m_client(client), m_callback(callback) {}
    virtual ~TnsJobAddressResolver() {
        m_callback = NULL;
        m_client = NULL;
    }
    virtual bool ResolveAddress(const std::string& path, std::vector<std::string>* addresses);

private:
    void OnJobNodeCreated(std::string path, int event);
    void OnTaskDataChanged(std::string path, int event);
    void OnTaskListChanged(std::string path, int event, std::vector<std::string> v);

private:
    Mutex m_mutex;
    // <Job: /zk/path, port>
    std::map<std::string, std::string> m_job_port;
    common::ZookeeperClient* m_client;
    AddressChangedCallback* m_callback;
};

} // namespace poppy

#endif // POPPY_ADDRESS_RESOLVER_TNS_JOB_RESOLVER_H
