// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu>

#ifndef POPPY_ADDRESS_RESOLVER_ZK_PATH_RESOLVER_H
#define POPPY_ADDRESS_RESOLVER_ZK_PATH_RESOLVER_H
#pragma once

#include <string>
#include <vector>
#include "common/system/concurrency/mutex.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "poppy/address_resolver/address_resolver.h"

namespace poppy {

class ZkAddressResolver : public AddressResolver
{
public:
    ZkAddressResolver(common::ZookeeperClient* client, AddressChangedCallback* callback) :
        m_client(client), m_callback(callback) {}
    virtual ~ZkAddressResolver() {
        m_callback = NULL;
        m_client = NULL;
    }
    virtual bool ResolveAddress(const std::string& path, std::vector<std::string>* addresses);

private:
    void OnNodeChanged(std::string path, int event);

private:
    Mutex m_mutex;
    // Full ZkAddress path: zk://path
    common::ZookeeperClient* m_client;
    AddressChangedCallback* m_callback;
    std::string m_zk_cluster;
};

} // namespace poppy

#endif // POPPY_ADDRESS_RESOLVER_ZK_PATH_RESOLVER_H
