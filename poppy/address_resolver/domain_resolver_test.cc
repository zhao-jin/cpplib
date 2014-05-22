// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <string>
#include <vector>
#include "poppy/address_resolver/resolver_factory.h"
#include "thirdparty/gtest/gtest.h"

TEST(ResolverFactory, DomainName)
{
    poppy::ResolverFactory resolver;
    std::vector<std::string> addresses;

    std::string domain_name = "tjd1.zk.oa.com:2181";
    ASSERT_TRUE(resolver.Resolve(domain_name, &addresses));
    ASSERT_EQ(5u, addresses.size());

    // There must be a port.
    domain_name = "tjd1.zk.oa.com";
    ASSERT_FALSE(resolver.Resolve(domain_name, &addresses));

    domain_name = "shit.zk.oa.com";
    ASSERT_FALSE(resolver.Resolve(domain_name, &addresses));
}

