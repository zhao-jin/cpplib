// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu>

#include "poppy/address_resolver/ip_list_resolver.h"

#include <algorithm>
#include "common/base/string/algorithm.h"
#include "common/system/net/socket.h"

namespace poppy {

bool IPAddressResolver::ResolveIPAddress(const std::string& path,
                                         std::vector<std::string>* addresses)
{
    addresses->clear();

    std::vector<std::string> vec;
    SplitString(path, ",",  &vec);

    SocketAddressInet socket_address;
    for (size_t i = 0; i < vec.size(); i++) {
        if (!socket_address.Parse(vec[i])) {
            return false;
        }
        addresses->push_back(vec[i]);
    }

    std::sort(addresses->begin(), addresses->end());
    std::vector<std::string>::iterator end_unique =
        unique(addresses->begin(), addresses->end());
    addresses->erase(end_unique, addresses->end());

    return true;
}

bool IPAddressResolver::ResolveAddress(const std::string& path,
                                       std::vector<std::string>* addresses)
{
    return ResolveIPAddress(path, addresses);
}

} // namespace poppy
