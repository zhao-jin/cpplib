// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <string>
#include <vector>
#include "common/base/closure.h"
#include "common/base/scoped_ptr.h"
#include "common/system/concurrency/this_thread.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "poppy/address_resolver/resolver_factory.h"
#include "thirdparty/gtest/gtest.h"

std::vector<std::string> addresses;

void OnAddressChanged(std::string name, std::vector<std::string> ips)
{
    addresses.assign(ips.begin(), ips.end());
}

TEST(ResolverFactory, DISABLED_ZookeeperPath)
{
    std::string path = "/zk/tjnl/xcube/_hsiaokangliu_";
    common::ZookeeperClient client;
    scoped_ptr<common::ZookeeperNode> node(client.Open(path));
    EXPECT_TRUE(node != NULL);
    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
    EXPECT_EQ(common::ZK_OK, node->Create());
    std::string ips = "10.168.151.77:2181,10.168.151.78:2181,10.168.151.79:2181";
    EXPECT_EQ(common::ZK_OK, node->SetContent(ips));

    poppy::ResolverFactory resolver(NewPermanentClosure(OnAddressChanged));
    std::string zk_address = "zk://tjnl.zk.oa.com:2181" + path;
    EXPECT_TRUE(resolver.Resolve(zk_address, &addresses));
    EXPECT_EQ(3u, addresses.size());

    ips = "10.168.151.77:2181,10.168.151.78:2181";
    EXPECT_EQ(common::ZK_OK, node->SetContent(ips));
    ThisThread::Sleep(1000);
    EXPECT_EQ(2u, addresses.size());

    ips = "10.168.151.77:2181";
    EXPECT_EQ(common::ZK_OK, node->SetContent(ips));
    ThisThread::Sleep(1000);
    EXPECT_EQ(1u, addresses.size());

    // if failed to resolve the address, just keep it unchanged.
    ips = "10.168.151.77";
    EXPECT_EQ(common::ZK_OK, node->SetContent(ips));
    ThisThread::Sleep(1000);
    EXPECT_EQ(1u, addresses.size());

    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
}

