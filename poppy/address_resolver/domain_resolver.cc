// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/address_resolver/domain_resolver.h"

#include "common/base/string/algorithm.h"
#include "common/base/string/string_number.h"
#include "common/system/net/domain_resolver.h"
#include "common/system/net/socket.h"

#include "thirdparty/glog/logging.h"

namespace poppy {

bool DomainResolver::ResolveAddress(const std::string& path, std::vector<std::string>* addresses)
{
    addresses->clear();

    std::vector<std::string> vec;
    SplitString(path, ":",  &vec);
    if (vec.size() != 2u) {
        return false;
    }

    std::string domain_name = vec[0];
    uint16_t port;
    if (!StringToNumber(vec[1], &port)) {
        return false;
    }

    int error;
    std::vector<IPAddress> ips;
    if (common::DomainResolver::ResolveIpAddress(domain_name, &ips, &error)) {
        for (size_t i = 0; i < ips.size(); ++i) {
            addresses->push_back(ips[i].ToString() + ":" + vec[1]);
        }
    } else {
        LOG(WARNING) << "Resolve domain name " << domain_name << " failed, "
            << common::DomainResolver::ErrorString(error);
        return false;
    }

    return !addresses->empty();
}

} // namespace poppy
