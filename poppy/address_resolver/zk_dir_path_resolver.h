// Copyright (c) 2013, Tencent Inc.
// All rights reserved.
//
// Author: Simon Wang <simonwang@tencent.com>
// Created: 03/15/13
// Description:

#ifndef POPPY_ADDRESS_RESOLVER_ZK_DIR_PATH_RESOLVER_H
#define POPPY_ADDRESS_RESOLVER_ZK_DIR_PATH_RESOLVER_H
#pragma once

#include <string>
#include <vector>
#include "common/system/concurrency/mutex.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "poppy/address_resolver/address_resolver.h"

namespace poppy {

class ZkDirAddressResolver : public AddressResolver
{
public:
    ZkDirAddressResolver(common::ZookeeperClient* client, AddressChangedCallback* callback) :
        m_client(client), m_callback(callback) {}
    virtual ~ZkDirAddressResolver() {
        m_callback = NULL;
        m_client = NULL;
    }
    virtual bool ResolveAddress(const std::string& path, std::vector<std::string>* addresses);

private:
    void OnChildrenChanged(std::string path,
                           int event,
                           std::vector<std::string> old_children);

private:
    Mutex m_mutex;
    common::ZookeeperClient* m_client;
    AddressChangedCallback* m_callback;
};

} // namespace poppy

#endif // POPPY_ADDRESS_RESOLVER_ZK_DIR_PATH_RESOLVER_H
