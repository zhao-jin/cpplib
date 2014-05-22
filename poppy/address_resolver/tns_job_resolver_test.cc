// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <algorithm>
#include <string>
#include <vector>
#include "common/base/closure.h"
#include "common/base/scoped_ptr.h"
#include "common/system/concurrency/this_thread.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "poppy/address_resolver/resolver_factory.h"
#include "thirdparty/gtest/gtest.h"

class TnsJobAddressResolverTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        job = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_";
        scoped_ptr<common::ZookeeperNode> node(client.Open(job));
        EXPECT_TRUE(node != NULL);
        EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
        EXPECT_EQ(common::ZK_OK, node->Create());
    }
    virtual void TearDown()
    {
        scoped_ptr<common::ZookeeperNode> node(client.Open(job));
        EXPECT_TRUE(node != NULL);
        EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
    }
    void OnAddressChanged(std::string name, std::vector<std::string> ips)
    {
        addresses.assign(ips.begin(), ips.end());
    }

protected:
    std::string job;
    common::ZookeeperClient client;
    std::vector<std::string> addresses;
};

TEST_F(TnsJobAddressResolverTest, DISABLED_TnsJobWithAnonymousPort)
{
    using namespace std;
    std::string task1 = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/0";
    scoped_ptr<common::ZookeeperNode> node1(client.Create(task1));
    EXPECT_TRUE(node1 != NULL);
    std::string content = "ip=127.0.0.1\nports=10000;10001";
    EXPECT_EQ(common::ZK_OK, node1->SetContent(content));

    std::string task2 = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/1";
    scoped_ptr<common::ZookeeperNode> node2(client.Create(task2));
    EXPECT_TRUE(node2 != NULL);
    content = "ip=192.168.1.1\nports=10000;10001";
    EXPECT_EQ(common::ZK_OK, node2->SetContent(content));

    // This path is given by user.
    // Fomart: /tns/<zk_cluster>-<tborg_cluster>/<role>/<jobname>
    std::string tns_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_";

    poppy::ResolverFactory resolver(NewPermanentClosure(
            this, &TnsJobAddressResolverTest::OnAddressChanged));
    EXPECT_TRUE(resolver.Resolve(tns_address, &addresses));
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:10000") != addresses.end());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "192.168.1.1:10000") != addresses.end());

    content = "ip=127.0.0.1\nports=20000;20001";
    EXPECT_EQ(common::ZK_OK, node1->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:20000") != addresses.end());

    EXPECT_EQ(common::ZK_OK, node2->Delete());
    ThisThread::Sleep(1000);
    ASSERT_EQ(1u, addresses.size());
    EXPECT_EQ("127.0.0.1:20000", addresses[0]);

    EXPECT_EQ(common::ZK_OK, node2->Create());
    content = "ip=192.168.1.1\nports=20000;20001";
    EXPECT_EQ(common::ZK_OK, node2->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:20000") != addresses.end());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "192.168.1.1:20000") != addresses.end());
}

TEST_F(TnsJobAddressResolverTest, DISABLED_TnsJobWithPortIndex)
{
    using namespace std;
    std::string task1 = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/0";
    scoped_ptr<common::ZookeeperNode> node1(client.Create(task1));
    EXPECT_TRUE(node1 != NULL);
    std::string content = "ip=127.0.0.1\nports=10000;10001";
    EXPECT_EQ(common::ZK_OK, node1->SetContent(content));

    std::string task2 = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/1";
    scoped_ptr<common::ZookeeperNode> node2(client.Create(task2));
    EXPECT_TRUE(node2 != NULL);
    content = "ip=192.168.1.1\nports=10000;10001";
    EXPECT_EQ(common::ZK_OK, node2->SetContent(content));

    // This path is given by user.
    // Fomart: /tns/<zk_cluster>-<tborg_cluster>/<role>/<jobname>?port=<number>
    std::string tns_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_?port=1";

    poppy::ResolverFactory resolver(NewPermanentClosure(
            this, &TnsJobAddressResolverTest::OnAddressChanged));
    EXPECT_TRUE(resolver.Resolve(tns_address, &addresses));
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:10001") != addresses.end());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "192.168.1.1:10001") != addresses.end());

    content = "ip=127.0.0.1\nports=20000;20001";
    EXPECT_EQ(common::ZK_OK, node1->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:20001") != addresses.end());

    EXPECT_EQ(common::ZK_OK, node2->Delete());
    ThisThread::Sleep(1000);
    ASSERT_EQ(1u, addresses.size());
    EXPECT_EQ("127.0.0.1:20001", addresses[0]);

    EXPECT_EQ(common::ZK_OK, node2->Create());
    content = "ip=192.168.1.1\nports=20000;20001";
    EXPECT_EQ(common::ZK_OK, node2->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:20001") != addresses.end());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "192.168.1.1:20001") != addresses.end());
}

TEST_F(TnsJobAddressResolverTest, DISABLED_TnsJobWithPortName)
{
    using namespace std;
    std::string task1 = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/0";
    scoped_ptr<common::ZookeeperNode> node1(client.Create(task1));
    EXPECT_TRUE(node1 != NULL);
    std::string content = "ip=127.0.0.1\nports=10000;10001\npoppy=10000\nxcube=10001";
    EXPECT_EQ(common::ZK_OK, node1->SetContent(content));

    std::string task2 = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/1";
    scoped_ptr<common::ZookeeperNode> node2(client.Create(task2));
    EXPECT_TRUE(node2 != NULL);
    content = "ip=192.168.1.1\nports=10000;10001\npoppy=10000\nxcube=10001";
    EXPECT_EQ(common::ZK_OK, node2->SetContent(content));

    // This path is given by user.
    // Fomart: /tns/<zk_cluster>-<tborg_cluster>/<role>/<jobname>?port=<name>
    std::string tns_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_?port=poppy";

    poppy::ResolverFactory resolver(NewPermanentClosure(
            this, &TnsJobAddressResolverTest::OnAddressChanged));
    std::string wrong_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_?name=poppy";
    EXPECT_FALSE(resolver.Resolve(wrong_address, &addresses));
    wrong_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_?port=not_exist";
    EXPECT_FALSE(resolver.Resolve(wrong_address, &addresses));

    EXPECT_TRUE(resolver.Resolve(tns_address, &addresses));
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:10000") != addresses.end());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "192.168.1.1:10000") != addresses.end());

    content = "ip=127.0.0.1\nports=20000;20001\npoppy=20000\nxcube=20001";
    EXPECT_EQ(common::ZK_OK, node1->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:20000") != addresses.end());

    EXPECT_EQ(common::ZK_OK, node2->Delete());
    ThisThread::Sleep(1000);
    ASSERT_EQ(1u, addresses.size());
    EXPECT_EQ("127.0.0.1:20000", addresses[0]);

    EXPECT_EQ(common::ZK_OK, node2->Create());
    content = "ip=192.168.1.1\nports=20000;20001\npoppy=20000\nxcube=20001";
    EXPECT_EQ(common::ZK_OK, node2->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:20000") != addresses.end());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "192.168.1.1:20000") != addresses.end());
}

TEST_F(TnsJobAddressResolverTest, DISABLED_RestartTnsJob)
{
    using namespace std;
    std::string task1 = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/0";
    scoped_ptr<common::ZookeeperNode> node1(client.Create(task1));
    EXPECT_TRUE(node1 != NULL);
    std::string content = "ip=127.0.0.1\nports=10000;10001";
    EXPECT_EQ(common::ZK_OK, node1->SetContent(content));

    std::string task2 = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/_hsiaokangliu_/1";
    scoped_ptr<common::ZookeeperNode> node2(client.Create(task2));
    EXPECT_TRUE(node2 != NULL);
    content = "ip=192.168.1.1\nports=10000;10001";
    EXPECT_EQ(common::ZK_OK, node2->SetContent(content));

    // This path is given by user.
    // Fomart: /tns/<zk_cluster>-<tborg_cluster>/<role>/<jobname>
    std::string tns_address = "/tns/tjnl-avivi/xcube/_hsiaokangliu_";

    poppy::ResolverFactory resolver(NewPermanentClosure(
            this, &TnsJobAddressResolverTest::OnAddressChanged));
    EXPECT_TRUE(resolver.Resolve(tns_address, &addresses));
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:10000") != addresses.end());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "192.168.1.1:10000") != addresses.end());

    scoped_ptr<common::ZookeeperNode> node(client.Open(job));
    EXPECT_TRUE(node != NULL);
    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
    ThisThread::Sleep(1000);
    ASSERT_EQ(0u, addresses.size());

    EXPECT_EQ(common::ZK_OK, node->Create());
    EXPECT_EQ(common::ZK_OK, node1->Create());
    EXPECT_EQ(common::ZK_OK, node2->Create());

    content = "ip=127.0.0.1\nports=20000;20001";
    EXPECT_EQ(common::ZK_OK, node1->SetContent(content));
    content = "ip=192.168.1.1\nports=20000;20001";
    EXPECT_EQ(common::ZK_OK, node2->SetContent(content));
    ThisThread::Sleep(1000);
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:20000") != addresses.end());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "192.168.1.1:20000") != addresses.end());
}

TEST_F(TnsJobAddressResolverTest, DISABLED_UnexistTnsJob)
{
    std::string job_path = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/__hsiaokangliu__";
    scoped_ptr<common::ZookeeperNode> node(client.Open(job_path));
    EXPECT_TRUE(node != NULL);
    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
    EXPECT_EQ(common::ZK_OK, node->Create());

    // This path is given by user.
    // Fomart: /tns/<zk_cluster>-<tborg_cluster>/<role>/<jobname>
    std::string tns_address = "/tns/tjnl-avivi/xcube/__hsiaokangliu__";
    poppy::ResolverFactory resolver(NewPermanentClosure(
            this, &TnsJobAddressResolverTest::OnAddressChanged));
    EXPECT_TRUE(resolver.Resolve(tns_address, &addresses));
    ASSERT_EQ(0u, addresses.size());

    std::string task1 = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/__hsiaokangliu__/0";
    scoped_ptr<common::ZookeeperNode> node1(client.Create(task1));
    EXPECT_TRUE(node1 != NULL);
    std::string content = "ip=127.0.0.1\nports=10000;10001";
    EXPECT_EQ(common::ZK_OK, node1->SetContent(content));

    std::string task2 = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/__hsiaokangliu__/1";
    scoped_ptr<common::ZookeeperNode> node2(client.Create(task2));
    EXPECT_TRUE(node2 != NULL);
    content = "ip=192.168.1.1\nports=10000;10001";
    EXPECT_EQ(common::ZK_OK, node2->SetContent(content));

    ThisThread::Sleep(1000);
    ASSERT_EQ(2u, addresses.size());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "127.0.0.1:10000") != addresses.end());
    EXPECT_TRUE(find(addresses.begin(), addresses.end(), "192.168.1.1:10000") != addresses.end());

    EXPECT_EQ(common::ZK_OK, node->RecursiveDelete());
    ThisThread::Sleep(1000);
    ASSERT_EQ(0u, addresses.size());
}

