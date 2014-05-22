// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu>

#ifndef POPPY_ADDRESS_RESOLVER_TNS_TASK_RESOLVER_H
#define POPPY_ADDRESS_RESOLVER_TNS_TASK_RESOLVER_H
#pragma once

#include <map>
#include <string>
#include <vector>
#include "common/system/concurrency/mutex.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "poppy/address_resolver/address_resolver.h"

namespace poppy {

class TnsTaskAddressResolver : public AddressResolver
{
public:
    TnsTaskAddressResolver(common::ZookeeperClient* client, AddressChangedCallback* callback) :
        m_client(client), m_callback(callback) {}
    virtual ~TnsTaskAddressResolver() {
        m_callback = NULL;
        m_client = NULL;
    }
    virtual bool ResolveAddress(const std::string& path, std::vector<std::string>* addresses);

private:
    void OnTaskChanged(std::string path, int event);

private:
    Mutex m_mutex;
    // <Task: /zk/path, port>
    std::map<std::string, std::string> m_task_port;
    common::ZookeeperClient* m_client;
    AddressChangedCallback* m_callback;
};

} // namespace poppy

#endif // POPPY_ADDRESS_RESOLVER_TNS_TASK_RESOLVER_H
