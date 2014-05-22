// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/address_resolver/resolver_factory.h"

#include "common/base/string/algorithm.h"
#include "common/system/net/socket.h"
#include "poppy/address_resolver/address_resolver.h"
#include "poppy/address_resolver/domain_resolver.h"
#include "poppy/address_resolver/ip_list_resolver.h"
#ifndef _WIN32
#include "poppy/address_resolver/tns_job_resolver.h"
#include "poppy/address_resolver/tns_task_resolver.h"
#include "poppy/address_resolver/zk_dir_path_resolver.h"
#include "poppy/address_resolver/zk_path_resolver.h"
#endif
#include "thirdparty/glog/logging.h"

namespace poppy {

ResolverFactory::~ResolverFactory()
{
    std::map<AddressType, AddressResolver*> resolvers;
    {
        MutexLocker locker(m_mutex);
        resolvers.swap(m_resolvers);
    }
#ifndef _WIN32
    m_client.Close();
#endif
    std::map<AddressType, AddressResolver*>::iterator iter;
    for (iter = resolvers.begin(); iter != resolvers.end(); ++iter) {
        delete iter->second;
    }
    delete m_callback;
    m_callback = NULL;
}

ResolverFactory::AddressType ResolverFactory::GetAddressType(const std::string& address)
{
    if (StringStartsWith(address, kZookeeperPrefix)) {
        return AT_ZK_PATH;
    } else if (StringStartsWith(address, kTnsPrefix)) {
        std::vector<std::string> v;
        SplitString(address, "/", &v);
        if (v.size() == 4u) {
            return AT_TNS_JOB;
        }
        return AT_TNS_TASK;
    } else if (StringStartsWith(address, kZookeeperDirPrefix)) {
        return AT_ZK_DIR_PATH;
    }

    std::string::size_type pos = address.find(",");
    if (pos == std::string::npos) {
        SocketAddressInet socket_address;
        if (!socket_address.Parse(address)) {
            return AT_DOMAIN;
        }
    }
    return AT_IP_LIST;
}

AddressResolver* ResolverFactory::GetResolver(const AddressType& type)
{
    MutexLocker locker(m_mutex);
    std::map<AddressType, AddressResolver*>::iterator iter = m_resolvers.find(type);
    if (iter != m_resolvers.end()) {
        return iter->second;
    } else {
        AddressResolver* rs = NULL;
        switch (type) {
        case AT_IP_LIST:
            rs = new IPAddressResolver();
            break;
        case AT_DOMAIN:
            rs = new DomainResolver();
            break;
#ifndef _WIN32
        case AT_ZK_PATH:
            rs = new ZkAddressResolver(&m_client, m_callback);
            break;
        case AT_TNS_JOB:
            rs = new TnsJobAddressResolver(&m_client, m_callback);
            break;
        case AT_TNS_TASK:
            rs = new TnsTaskAddressResolver(&m_client, m_callback);
            break;
        case AT_ZK_DIR_PATH:
            rs = new ZkDirAddressResolver(&m_client, m_callback);
            break;
#endif
        default:
            return NULL;
        }
        m_resolvers[type] = rs;
        return rs;
    }
    return NULL;
}

bool ResolverFactory::Resolve(const std::string& path, std::vector<std::string>* addresses)
{
    AddressType type = GetAddressType(path);
    AddressResolver* resolver = GetResolver(type);
    return resolver->ResolveAddress(path, addresses);
}

} // namespace poppy
