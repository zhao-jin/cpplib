// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <string>
#include <vector>
#include "common/base/scoped_ptr.h"
#include "common/base/string/string_number.h"
#include "common/system/io/file.h"
#include "common/zookeeper/client/tests/test_helper.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "common/zookeeper/client/zookeeper_session.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/gtest/gtest.h"

namespace common {

static const std::string kPath = GetTestNodePath();

TEST(ZookeeperClient, NoNodeTest)
{
    ZookeeperClient client;
    std::string path = "/zk/ci/xcube/xxx/xxx/xxx";
    int error_code = 0;
    scoped_ptr<ZookeeperNode> node(client.Create(path, "", 0, &error_code));
    if (node == NULL) {
        LOG(ERROR) << "failed to create node: " << path << ", error: " << error_code;
        return;
    } else {
        LOG(INFO) << node->GetPath();
    }
}

TEST(ZookeeperClient, NodeOperationTest)
{
    ZookeeperClient client;
    std::string path = "/zk";
    ASSERT_EQ(NULL, client.Open(path));

    scoped_ptr<ZookeeperNode> node(client.Open(kPath));
    ASSERT_TRUE(node != NULL);
    EXPECT_EQ(ZK_OK, node->RecursiveDelete());

    EXPECT_EQ(ZK_NONODE, node->Exists());

    int error_code = node->Create();
    EXPECT_EQ(ZK_OK, error_code);
    ASSERT_EQ(ZK_OK, node->Exists());

    std::string test_string = "hi, hsiaokangliu, this is a test!";
    ASSERT_EQ(ZK_OK, node->SetContent(test_string));

    std::string contents;
    std::vector<std::string> jsons;
    Stat stat;
    ASSERT_EQ(ZK_OK, node->GetContent(&contents, &stat));
    ASSERT_EQ(contents, test_string);
    ASSERT_EQ(ZK_OK, node->GetContent(&contents, NULL));
    ASSERT_EQ(contents, test_string);
    ASSERT_EQ(ZK_OK, node->GetContentAsJson(&jsons));

    ASSERT_EQ(ZK_NONODE, node->SetChildContent("child", "child"));

    for (size_t i = 0; i < 10; i++) {
        std::string name = IntegerToString(i);
        std::string child_path = kPath + "/" + name;
        scoped_ptr<ZookeeperNode> child_node(client.Open(child_path));
        ASSERT_TRUE(child_node != NULL);
        ASSERT_EQ(ZK_OK, child_node->Create());
        ASSERT_EQ(ZK_OK, node->SetChildContent(name, name));

        ASSERT_EQ(ZK_OK, node->GetChildContent(name, &contents));
        ASSERT_EQ(name, contents);
    }
    std::vector<std::string> children;
    EXPECT_EQ(ZK_OK, node->GetChildren(&children));
    EXPECT_EQ(10u, children.size());
    for (size_t i = 0; i < children.size(); ++i) {
        LOG(INFO) << children[i];
    }

    std::string child_path = kPath + "/0";
    scoped_ptr<ZookeeperNode> child_node(client.Open(child_path));
    ASSERT_TRUE(child_node != NULL);

    ASSERT_EQ(ZK_OK, child_node->Delete());
    ASSERT_EQ(ZK_NONODE, child_node->Exists());

    ASSERT_EQ(ZK_OK, node->RecursiveDelete());
    ASSERT_EQ(ZK_NONODE, node->Exists());

    path = "/zk/ci/hsiaokangliu_test/yy/zz/aa/bb";
    node.reset(client.Open(path));
    ASSERT_TRUE(node != NULL);
    ASSERT_EQ(ZK_OK, node->RecursiveCreate("abc"));
    ASSERT_EQ(ZK_OK, node->Exists());
    ASSERT_EQ(ZK_OK, node->GetContent(&contents, NULL));
    ASSERT_EQ("abc", contents);
    path = "/zk/ci/hsiaokangliu_test";
    node.reset(client.Open(path));
    ASSERT_TRUE(node != NULL);
    ASSERT_EQ(ZK_OK, node->RecursiveDelete());
    ASSERT_EQ(ZK_NONODE, node->Exists());
}

TEST(ZookeeperClient, FullPathNodeOperationTest)
{
    ZookeeperClient client;
    std::string path = "abc";
    ASSERT_EQ(NULL, client.OpenWithFullPath(path));

    path = "10.12.22.94:2181,10.12.22.80:2181,10.12.22.95:2181" + kPath;
    scoped_ptr<ZookeeperNode> node(client.OpenWithFullPath(path));
    ASSERT_TRUE(node != NULL);
    EXPECT_EQ(ZK_OK, node->RecursiveDelete());
    EXPECT_EQ(ZK_NONODE, node->Exists());

    int error_code = node->Create();
    EXPECT_EQ(ZK_OK, error_code);
    ASSERT_EQ(ZK_OK, node->Exists());

    std::string test_string = "hi, hsiaokangliu, this is a test!";
    ASSERT_EQ(ZK_OK, node->SetContent(test_string));

    std::string contents;
    Stat stat;
    ASSERT_EQ(ZK_OK, node->GetContent(&contents, &stat));
    ASSERT_EQ(contents, test_string);
    ASSERT_EQ(ZK_OK, node->GetContent(&contents, NULL));
    ASSERT_EQ(contents, test_string);

    ASSERT_EQ(ZK_NONODE, node->SetChildContent("child", "child"));

    for (size_t i = 0; i < 10; i++) {
        std::string name = IntegerToString(i);
        std::string child_path = path + "/" + name;
        scoped_ptr<ZookeeperNode> child_node(client.OpenWithFullPath(child_path));
        ASSERT_TRUE(child_node != NULL);
        ASSERT_EQ(ZK_OK, child_node->Create());
        ASSERT_EQ(ZK_OK, node->SetChildContent(name, name));

        ASSERT_EQ(ZK_OK, node->GetChildContent(name, &contents));
        ASSERT_EQ(name, contents);
    }
    std::vector<std::string> children;
    EXPECT_EQ(ZK_OK, node->GetChildren(&children));
    EXPECT_EQ(10u, children.size());
    for (size_t i = 0; i < children.size(); ++i) {
        LOG(INFO) << children[i];
    }

    std::string child_path = path + "/0";
    scoped_ptr<ZookeeperNode> child_node(client.OpenWithFullPath(child_path));
    ASSERT_TRUE(child_node != NULL);

    ASSERT_EQ(ZK_OK, child_node->Delete());
    ASSERT_EQ(ZK_NONODE, child_node->Exists());

    ASSERT_EQ(ZK_OK, node->RecursiveDelete());
    ASSERT_EQ(ZK_NONODE, node->Exists());
}

TEST(ZookeeperClient, InvalidSessionTest)
{
    ZookeeperSession session("invalidcluster", ZookeeperClient::Options(), NULL);
    std::string path = "zk/ci/xcube";
    ASSERT_EQ(NULL, session.Open(path, NULL));
    ASSERT_EQ(ZK_INVALIDSTATE, session.Create(path.c_str(), NULL, 0, NULL, 0, NULL, 0));
    ASSERT_EQ(ZK_INVALIDSTATE, session.Get(0, 0, 0, 0, 0));
    ASSERT_EQ(ZK_INVALIDSTATE, session.Set(0, 0, 0, 0));
    ASSERT_EQ(ZK_INVALIDSTATE, session.Exists(0, 0, 0));
    ASSERT_EQ(ZK_INVALIDSTATE, session.GetChildren(0, 0, 0));
    ASSERT_EQ(ZK_INVALIDSTATE, session.GetAcl(0, 0, 0));
    ASSERT_EQ(ZK_INVALIDSTATE, session.SetAcl(0, 0, 0));
    ASSERT_EQ(ZK_INVALIDSTATE, session.Delete(0, 0));
    ASSERT_EQ(ZK_INVALIDSTATE, session.WatcherGet(0, 0, 0, 0, 0, 0));
    ASSERT_EQ(ZK_INVALIDSTATE, session.WatcherExists(0, 0, 0, 0));
    ASSERT_EQ(ZK_INVALIDSTATE, session.WatcherGetChildren(0, 0, 0, 0));
}

} // namespace common
