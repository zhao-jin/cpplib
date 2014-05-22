// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef POPPY_ADDRESS_RESOLVER_RESOLVER_FACTORY_H
#define POPPY_ADDRESS_RESOLVER_RESOLVER_FACTORY_H

#include <map>
#include <string>
#include <vector>
#include "common/system/concurrency/mutex.h"
#ifndef _WIN32
#include "common/zookeeper/client/zookeeper_client.h"
#endif
#include "poppy/address_resolver/address_resolver.h"

namespace poppy {

class ResolverFactory
{
public:
    explicit ResolverFactory(AddressChangedCallback* callback = NULL) : m_callback(callback) {}
    ~ResolverFactory();

    bool Resolve(const std::string& path, std::vector<std::string>* addresses);

private:
    enum AddressType
    {
        AT_DOMAIN,
        AT_IP_LIST,
        AT_ZK_DIR_PATH,
        AT_ZK_PATH,
        AT_TNS_TASK,
        AT_TNS_JOB,
    };
    static AddressType GetAddressType(const std::string& path);
    AddressResolver* GetResolver(const AddressType& type);

private:
    Mutex m_mutex;
    AddressChangedCallback* m_callback;
#ifndef _WIN32
    common::ZookeeperClient m_client;
#endif
    std::map<AddressType, AddressResolver*> m_resolvers;
};

} // namespace poppy

#endif // POPPY_ADDRESS_RESOLVER_RESOLVER_FACTORY_H
