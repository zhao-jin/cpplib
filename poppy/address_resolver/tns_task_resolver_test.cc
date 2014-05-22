// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "common/base/closure.h"
#include "common/base/scoped_ptr.h"
#include "common/system/concurrency/this_thread.h"
#include "poppy/address_resolver/resolver_factory.h"
#include "thirdparty/gtest/gtest.h"

std::vector<std::string> addresses;

void OnAddressChanged(std::string name, std::vector<std::string> ips)
{
    addresses.assign(ips.begin(), ips.end());
}

TEST(ResolverFactory, DISABLED_TnsTaskWithAnonymousPort)
{
    std::string job = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_";
    common::ZookeeperClient client;
    scoped_ptr<common::ZookeeperNode> node(client.Open(job));
    EXPECT_TRUE(node != NULL);
    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
    EXPECT_EQ(common::ZK_OK, node->Create());

    std::string task = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/0";
    node.reset(client.Create(task));
    EXPECT_TRUE(node != NULL);

    std::string content = "ip=127.0.0.1\nports=10000;10001";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));

    // This path is given by user.
    // path format: /tns/<zk_cluster>-<tborg_cluster>/<role>/<jobname>/<taskid>
    std::string tns_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_/0";

    poppy::ResolverFactory resolver(NewPermanentClosure(OnAddressChanged));

    std::string wrong_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_/0?name=poppy";
    EXPECT_FALSE(resolver.Resolve(wrong_address, &addresses));
    wrong_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_/0?port=not_exist";
    EXPECT_FALSE(resolver.Resolve(wrong_address, &addresses));

    EXPECT_TRUE(resolver.Resolve(tns_address, &addresses));
    ASSERT_EQ(1u, addresses.size());
    EXPECT_EQ("127.0.0.1:10000", addresses[0]);

    content = "ip=127.0.0.1\nports=20000;20001";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(1u, addresses.size());
    ASSERT_EQ("127.0.0.1:20000", addresses[0]);

    content = "ip=127.0.0.1\nports=88888";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(1u, addresses.size());
    ASSERT_EQ("127.0.0.1:20000", addresses[0]);

    content = "ip=255.255.255.255\n";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(0u, addresses.size());

    node.reset(client.Open(job));
    EXPECT_TRUE(node != NULL);
    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
}

TEST(ResolverFactory, DISABLED_TnsTaskWithPortIndex)
{
    std::string job = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_";
    common::ZookeeperClient client;
    scoped_ptr<common::ZookeeperNode> node(client.Open(job));
    EXPECT_TRUE(node != NULL);
    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
    EXPECT_EQ(common::ZK_OK, node->Create());

    std::string task = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/0";
    node.reset(client.Create(task));
    EXPECT_TRUE(node != NULL);

    std::string content = "ip=127.0.0.1\nports=10000;10001";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));

    // This path is given by user.
    // path format: /tns/<zk_cluster>-<tborg_cluster>/<role>/<jobname>/<taskid>?port=<number>
    std::string tns_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_/0?port=1";

    poppy::ResolverFactory resolver(NewPermanentClosure(OnAddressChanged));
    EXPECT_TRUE(resolver.Resolve(tns_address, &addresses));
    ASSERT_EQ(1u, addresses.size());
    EXPECT_EQ("127.0.0.1:10001", addresses[0]);

    content = "ip=127.0.0.1\nports=20000;20001";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(1u, addresses.size());
    ASSERT_EQ("127.0.0.1:20001", addresses[0]);

    content = "ip=127.0.0.1\nports=66666;88888";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(1u, addresses.size());
    ASSERT_EQ("127.0.0.1:20001", addresses[0]);

    content = "ip=255.255.255.255\n";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(0u, addresses.size());

    node.reset(client.Open(job));
    EXPECT_TRUE(node != NULL);
    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
}

TEST(ResolverFactory, DISABLED_TnsTaskWithPortName)
{
    std::string job = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_";
    common::ZookeeperClient client;
    scoped_ptr<common::ZookeeperNode> node(client.Open(job));
    EXPECT_TRUE(node != NULL);
    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
    EXPECT_EQ(common::ZK_OK, node->Create());

    std::string task = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/0";
    node.reset(client.Create(task));
    EXPECT_TRUE(node != NULL);

    std::string content = "ip=127.0.0.1\nports=30000;30001\npoppy=10000\nxcube=10001";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));

    poppy::ResolverFactory resolver(NewPermanentClosure(OnAddressChanged));
    std::string tns_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_/0?port=poppy";
    EXPECT_TRUE(resolver.Resolve(tns_address, &addresses));

    ASSERT_EQ(1u, addresses.size());
    EXPECT_EQ("127.0.0.1:10000", addresses[0]);

    content = "ip=127.0.0.1\nports=40000;40001\npoppy=20000\nxcube=20001";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(1u, addresses.size());
    EXPECT_EQ("127.0.0.1:20000", addresses[0]);

    node.reset(client.Open(job));
    EXPECT_TRUE(node != NULL);
    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
}

TEST(ResolverFactory, DISABLED_RestartTnsTask)
{
    std::string job = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_";
    common::ZookeeperClient client;
    scoped_ptr<common::ZookeeperNode> node(client.Open(job));
    EXPECT_TRUE(node != NULL);
    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
    EXPECT_EQ(common::ZK_OK, node->Create());

    std::string task = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/0";
    node.reset(client.Create(task));
    EXPECT_TRUE(node != NULL);

    std::string content = "ip=127.0.0.1\nports=10000;10001";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));

    // This path is given by user.
    // path format: /tns/<zk_cluster>-<tborg_cluster>/<role>/<jobname>/<taskid>
    std::string tns_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_/0";

    poppy::ResolverFactory resolver(NewPermanentClosure(OnAddressChanged));
    EXPECT_TRUE(resolver.Resolve(tns_address, &addresses));
    ASSERT_EQ(1u, addresses.size());
    EXPECT_EQ("127.0.0.1:10000", addresses[0]);

    content = "ip=127.0.0.1\nports=20000;20001";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(1u, addresses.size());
    ASSERT_EQ("127.0.0.1:20000", addresses[0]);

    // Delete task and then create it again.
    EXPECT_EQ(common::ZK_OK, node->Delete());
    EXPECT_EQ(common::ZK_OK, node->Create());
    content = "ip=127.0.0.1\nports=30000;30001";
    EXPECT_EQ(common::ZK_OK, node->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(1u, addresses.size());
    ASSERT_EQ("127.0.0.1:30000", addresses[0]);

    node.reset(client.Open(job));
    EXPECT_TRUE(node != NULL);
    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
}

