// Copyright (c) 2013, Tencent Inc.
// All rights reserved.
//
// Author: Simon Wang <simonwang@tencent.com>
// Created: 03/15/13
// Description:

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

class ZkDirPathResolverTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        m_path = "/zk/ci/xcube/_simonwang_";
        m_client_1.reset(new common::ZookeeperClient());
        m_client_2.reset(new common::ZookeeperClient());
        m_client_3.reset(new common::ZookeeperClient());

        scoped_ptr<common::ZookeeperNode> node(m_client_1->Create(m_path));
        ASSERT_TRUE(node != NULL);
        // create children node
        scoped_ptr<common::ZookeeperNode> children_node(m_client_1->Create(m_path + "/children_1",
                                                                           "10.168.151.77:2181",
                                                                           ZOO_EPHEMERAL));
        ASSERT_TRUE(children_node != NULL);
        children_node.reset(m_client_2->Create(m_path + "/children_2",
                                               "10.168.151.78:2181",
                                               ZOO_EPHEMERAL));
        ASSERT_TRUE(children_node != NULL);

        children_node.reset(m_client_3->Create(m_path + "/children_3",
                                               "10.168.151.76:2181",
                                               ZOO_EPHEMERAL));
        ASSERT_TRUE(children_node != NULL);
    }

    virtual void TearDown()
    {
    }

protected:
    std::string m_path;

    scoped_ptr<common::ZookeeperClient> m_client_1;
    scoped_ptr<common::ZookeeperClient> m_client_2;
    scoped_ptr<common::ZookeeperClient> m_client_3;
};

TEST_F(ZkDirPathResolverTest, Empty)
{
    m_client_1.reset();
    m_client_2.reset();
    m_client_3.reset();

    poppy::ResolverFactory resolver(NewPermanentClosure(OnAddressChanged));
    std::string zk_address = "zkdir://ci.zk.oa.com:2181" + m_path;
    EXPECT_FALSE(resolver.Resolve(zk_address, &addresses));
}

TEST_F(ZkDirPathResolverTest, Normal)
{
    poppy::ResolverFactory resolver(NewPermanentClosure(OnAddressChanged));
    std::string zk_address = "zkdir://ci.zk.oa.com:2181" + m_path;
    EXPECT_TRUE(resolver.Resolve(zk_address, &addresses));
}

TEST_F(ZkDirPathResolverTest, NodeLeave)
{
    poppy::ResolverFactory resolver(NewPermanentClosure(OnAddressChanged));
    std::string zk_address = "zkdir://ci.zk.oa.com:2181" + m_path;
    EXPECT_TRUE(resolver.Resolve(zk_address, &addresses));

    m_client_1.reset();
    ThisThread::Sleep(1000);
    EXPECT_EQ(2u, addresses.size());
    for (size_t i = 0; i < addresses.size(); ++i) {
        VLOG(3) << addresses[i];
    }

    m_client_2.reset();
    ThisThread::Sleep(1000);
    EXPECT_EQ(1u, addresses.size());
    for (size_t i = 0; i < addresses.size(); ++i) {
        VLOG(3) << addresses[i];
    }
}

TEST_F(ZkDirPathResolverTest, NodeAdd)
{
    poppy::ResolverFactory resolver(NewPermanentClosure(OnAddressChanged));
    std::string zk_address = "zkdir://ci.zk.oa.com:2181" + m_path;
    EXPECT_TRUE(resolver.Resolve(zk_address, &addresses));

    common::ZookeeperClient client;
    scoped_ptr<common::ZookeeperNode> children_node(client.Create(m_path + "/children_4",
                                                                  "10.168.151.80:2181",
                                                                  ZOO_EPHEMERAL));

    ThisThread::Sleep(1000);
    EXPECT_EQ(4u, addresses.size());
    for (size_t i = 0; i < addresses.size(); ++i) {
        VLOG(3) << addresses[i];
    }
}
