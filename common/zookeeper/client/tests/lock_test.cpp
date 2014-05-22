// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <string>
#include "common/base/scoped_ptr.h"
#include "common/system/concurrency/base_thread.h"
#include "common/system/concurrency/this_thread.h"
#include "common/zookeeper/client/tests/test_helper.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/gtest/gtest.h"

namespace common {

static const std::string kPath = GetTestNodePath();

class AsyncLockThread : public BaseThread {
public:
    void OnLocked(std::string path, int state)
    {
        if (state == ZK_OK) {
            LOG(INFO) << "AsyncLockThread locked path: " << path;
        } else {
            LOG(INFO) << "AsyncLockThread failed to lock path with error code: " << state;
        }
        LOG(INFO) << "AsyncLockThread releases lock";
        Close();
    }

    void Entry()
    {
        scoped_ptr<ZookeeperNode> node(m_client.Create(kPath));
        ASSERT_TRUE(node != NULL);

        ZookeeperNode::WatcherCallback* closure = NewClosure(this, &AsyncLockThread::OnLocked);
        node->AsyncLock(closure);
        LOG(INFO) << "AsyncLockThread try to lock node: " << kPath;
    }

    void Close()
    {
        m_client.Close();
    }

private:
    ZookeeperClient m_client;
};

class BlockingLockThread : public BaseThread {
public:
    void Entry()
    {
        scoped_ptr<ZookeeperNode> node(m_client.Create(kPath));
        ASSERT_TRUE(node != NULL);

        int error_code = node->Lock();
        if (error_code != ZK_OK) {
            LOG(INFO) << "BlockingLockThread failed to lock, error_code = " << error_code;
        } else {
            LOG(INFO) << "BlockingLockThread locked node: " << node->GetPath();
        }
    }

    void Close()
    {
        m_client.Close();
    }

private:
    ZookeeperClient m_client;
};

TEST(LockTest, MutiThreadLock)
{
    ZookeeperClient client;
    AsyncLockThread t1;
    BlockingLockThread t2;

    scoped_ptr<ZookeeperNode> node(client.Create(kPath));
    ASSERT_TRUE(node != NULL);

    EXPECT_EQ(ZK_OK, node->RecursiveDelete());
    EXPECT_EQ(ZK_OK, node->Create());

    ASSERT_FALSE(node->IsLocked());
    ASSERT_EQ(ZK_OK, node->Lock());
    ASSERT_TRUE(node->IsLocked());
    ASSERT_EQ(ZK_OK, node->Unlock());
    ASSERT_FALSE(node->IsLocked());

    ASSERT_EQ(ZK_OK, node->TryLock());
    LOG(INFO) << "MainThread locked path: " << kPath;
    ASSERT_TRUE(node->IsLocked());

    t1.Start();
    t2.Start();
    ThisThread::Sleep(1000);

    ASSERT_TRUE(node->IsLocked());
    ASSERT_EQ(ZK_OK, node->Unlock());
    LOG(INFO) << "Main thread released lock.";
    ThisThread::Sleep(2000);
    ASSERT_TRUE(node->IsLocked());

    t1.Join();
    t2.Join();

    ASSERT_TRUE(node->IsLocked());
    t2.Close();
    ThisThread::Sleep(1000);
    ASSERT_FALSE(node->IsLocked());

    ASSERT_EQ(ZK_OK, node->Lock());
    ASSERT_TRUE(node->IsLocked());
    ASSERT_EQ(ZK_OK, node->Unlock());

    ASSERT_EQ(ZK_OK, node->RecursiveDelete());
}

} // namespace common
