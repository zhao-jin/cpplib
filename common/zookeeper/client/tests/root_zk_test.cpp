// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <string>
#include <vector>
#include "common/base/scoped_ptr.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "common/zookeeper/client/zookeeper_session.h"
#include "thirdparty/gtest/gtest.h"

namespace common {

DECLARE_bool(enable_test_hosts);

TEST(ZookeeperClient, RootZkTest)
{
    ZookeeperClient client;

    std::string path = "/zk/xatc/xcube";

    int error_code = 0;
    scoped_ptr<ZookeeperNode> node(client.Open(path, &error_code));
    ASSERT_TRUE(node == NULL);
    ASSERT_EQ(ZK_DISCONNECTED, error_code);
}

TEST(ZookeeperClient, DnsFailureTest)
{
    FLAGS_enable_test_hosts = true;

    std::string cluster = "dns_failure.zk.oa.com:2181";
    std::string ips;
    ASSERT_TRUE(ZookeeperSession::ResolveServerAddress(cluster, &ips));
}

} // namespace common
