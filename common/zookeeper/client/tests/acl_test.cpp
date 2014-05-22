// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <iostream>
#include <string>
#include <vector>
#include "common/base/scoped_ptr.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "common/zookeeper/client/zookeeper_node.h"
#include "thirdparty/gtest/gtest.h"

namespace common {

TEST(ZookeeperTest, Acl)
{
    ZookeeperClient client;
    scoped_ptr<ZookeeperNode> node(client.Open("/zk/xaec"));
    ASSERT_TRUE(node != NULL);
    ZookeeperAcl acls;
    ASSERT_EQ(ZK_OK, node->GetAcl(&acls));

    for (ZookeeperAcl::iterator iter = acls.begin(); iter != acls.end(); ++iter) {
        std::cout << (*iter).scheme << "\t " << (*iter).id << "\t" << (*iter).permission << "\n";
    }
}

} // namespace common
