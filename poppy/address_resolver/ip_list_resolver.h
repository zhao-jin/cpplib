// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu>

#ifndef POPPY_ADDRESS_RESOLVER_IP_LIST_RESOLVER_H
#define POPPY_ADDRESS_RESOLVER_IP_LIST_RESOLVER_H
#pragma once

#include <string>
#include <vector>
#include "poppy/address_resolver/address_resolver.h"

namespace poppy {

class IPAddressResolver : public AddressResolver
{
public:
    IPAddressResolver() {}
    virtual ~IPAddressResolver() {}

    static bool ResolveIPAddress(const std::string& path, std::vector<std::string>* addresses);

    virtual bool ResolveAddress(const std::string& path, std::vector<std::string>* addresses);
};

} // namespace poppy

#endif // POPPY_ADDRESS_RESOLVER_IP_LIST_RESOLVER_H
