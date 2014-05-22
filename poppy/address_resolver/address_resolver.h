// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu>

#ifndef POPPY_ADDRESS_RESOLVER_ADDRESS_RESOLVER_H
#define POPPY_ADDRESS_RESOLVER_ADDRESS_RESOLVER_H

#include <string>
#include <vector>
#include "common/base/closure.h"
#include "common/system/concurrency/mutex.h"
#ifndef _WIN32
#include "common/zookeeper/client/zookeeper_client.h"
#endif

namespace poppy {

static const char kTnsPrefix[] = "/tns/";
static const char kZookeeperPrefix[] = "zk://";
static const char kZookeeperDirPrefix[] = "zkdir://";

typedef Closure<void, std::string, std::vector<std::string> > AddressChangedCallback;

class AddressResolver
{
public:
    AddressResolver() {}
    virtual ~AddressResolver() {}
    virtual bool ResolveAddress(const std::string& path, std::vector<std::string>* addresses) = 0;
};

} // namespace poppy

#endif // POPPY_ADDRESS_RESOLVER_ADDRESS_RESOLVER_H

