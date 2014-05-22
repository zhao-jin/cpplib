// Copyright 2012, Tencent Inc.
// Author: Rui Li <rli@tencent.com>
//         Xiaokang Liu <hsiaokangliu@tencent.com>
//
// Defines a mock class for ZookeeperNode,
// and a manager contains the instances of ZookeeperNodeMock with their pathes as key.
//
// HOW TO WRITE CLIENT CODE:
//
// You write zookeeper client code regardless it is to talk with a real
// server or a mock server. The only difference is that a mock server address starts
// with "/zk/mock/".
//
// HOWT TO WRITE TEST CODE:
//
// You can specify expected behavior using EXPECT_ZK_CALL.
// EXPECT_ZK_CALL requires the ZookeeperNodeMock instance as the first input parameter.
//
// E.g. If you want to change the return value of SetContent:
//
// scoped_ptr<ZookeeperNode> node(client.Open("/zk/mock/test_node"));
// EXPECT_ZK_CALL(*node, SetContent(_,_))
//     .Times(1)
//     .WillOnce(Return(ZK_DISCONNECTED));
//
// Then you can create the node with ZookeeperClient and do the operations:
//
// EXPECT_EQ(ZK_DISCONNECTED, node->SetContent(value));
//
// For more example of gmock, please refer to http://code.google.com/p/googlemock/
//
// A HANDY WAY TO MOCK:
//
// You also can specify expected behavior using a special path like /zk/mock/ZK_xxx.
// ZK_xxx is an error code defined in common/zookeeper/client/zookeeper_types.h
//
// E.g If you want your zookeeper mock node return a value of ZK_NONODE, your code may be like:
//
// scoped_ptr<ZookeeperNode> node(client.Open("/zk/mock/ZK_NONODE"));
// EXPECT_EQ(ZK_NONODE, node->SetContent(value));
//
// Then you can do operations defined in ZookeeperNode, and the return value will be ZK_NONODE.
//
// For more example, please see zookeeper_node_mock_test.cc

#ifndef COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_NODE_MOCK_H
#define COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_NODE_MOCK_H

#include <map>
#include <string>
#include <vector>
#include "common/base/singleton.h"
#include "common/zookeeper/client/zookeeper_node.h"
#include "common/zookeeper/client/zookeeper_types.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/gmock/gmock.h"

namespace common {

static const std::string kMockContent = "/mock/";

class ZookeeperNodeMock : public ZookeeperNode {
public:
    ZookeeperNodeMock(int ret_code = ZK_OK)
    {
        m_default_ret_code = ret_code;
        SetDefaultBehavior();
    }

    MOCK_METHOD2(Create,
        int(const std::string& data, int flags));
    MOCK_METHOD2(RecursiveCreate,
        int(const std::string& data, int flags));
    MOCK_METHOD3(GetContent,
        int(std::string* contents, Stat* stat, WatcherCallback* callback));
    MOCK_METHOD1(GetContentAsJson,
        int(std::vector<std::string>* contents));
    MOCK_METHOD2(SetContent,
        int(const std::string& value, int version));
    MOCK_METHOD1(Exists,
        int(WatcherCallback* callback));
    MOCK_METHOD1(Delete,
        int(int version));
    MOCK_METHOD0(RecursiveDelete,
        int());
    MOCK_METHOD3(GetChildContent,
        int(const std::string& child_name, std::string* contents, Stat* stat));
    MOCK_METHOD3(SetChildContent,
        int(const std::string& child_name, const std::string& contents, int version));
    MOCK_METHOD2(GetChildren,
        int(std::vector<std::string>* children, ChildrenWatcherCallback* callback));
    MOCK_METHOD1(IsLocked,
        bool(int* error_code));
    MOCK_METHOD0(Lock,
        int());
    MOCK_METHOD0(TryLock,
        int());
    MOCK_METHOD1(AsyncLock,
        void(Closure<void, std::string, int>* callback));
    MOCK_METHOD0(Unlock,
        int());

private:
    void SetDefaultBehavior();

private:
    int m_default_ret_code;
};

ZookeeperNodeMock* CastToZookeeperNodeMock(ZookeeperNode* node,
                                           const char* file,
                                           int line);

#define EXPECT_ZK_CALL(node, call) \
    EXPECT_CALL(*(::common::CastToZookeeperNodeMock(&node, __FILE__, __LINE__)), \
                call)

} // namespace common

#endif // COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_NODE_MOCK_H
