// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <string>
#include <vector>
#include "poppy/address_resolver/resolver_factory.h"
#include "thirdparty/gtest/gtest.h"

TEST(ResolverFactory, IPAddress)
{
    poppy::ResolverFactory resolver;
    std::vector<std::string> addresses;

    std::string ips = "10.168.151.77:2181,10.168.151.78:2181,10.168.151.79:2181";
    ASSERT_TRUE(resolver.Resolve(ips, &addresses));
    ASSERT_EQ(3u, addresses.size());

    ips = "10.168.151.77:2181,10.168.151.78:2181,10.168.151.79:2181,";
    ASSERT_TRUE(resolver.Resolve(ips, &addresses));
    ASSERT_EQ(3u, addresses.size());

    ips = "10.168.151.77:2181,10.168.151.78:2181,10.168.151.79";
    ASSERT_FALSE(resolver.Resolve(ips, &addresses));
}
