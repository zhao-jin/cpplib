// Copyright (c) 2012, Tencent Inc.
// All rights reserved.
//
// Author: rabbitliu <rabbitliu@tencent.com>
// Created: 05/02/12
// Description:

#include <string>
#include "common/base/scoped_ptr.h"
#include "common/crypto/credential/generator.h"
#include "common/crypto/random/true_random.h"
#include "common/system/concurrency/base_thread.h"
#include "common/system/concurrency/event.h"
#include "taas/sdk/client_utility.h"
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/gtest/gtest.h"
#include "thirdparty/zookeeper/zookeeper.h"

DEFINE_string(host, "tjnl.zk.oa.com:2181", "zk host for acl test.");
DEFINE_string(login_identity, "rabbitliu", "login identity.");
DEFINE_string(login_role, "zk_test", "login role.");
DEFINE_int32(thread_num, 3, "thread_num for test.");

void GlobalWatcher(zhandle_t* zh, int type, int state, const char *path, void *context)
{
    LOG(INFO) << "event type:" << type << " , session state: " << state;
    AutoResetEvent* sync_event = static_cast<AutoResetEvent*>(context);
    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_EXPIRED_SESSION_STATE) {
            sync_event->Set();
            LOG(FATAL) << "zoo session expired.";
        } else if (state == ZOO_CONNECTED_STATE) {
            sync_event->Set();
        }
    }
}

class QuotaTestThread : public BaseThread
{
public:
    explicit QuotaTestThread(int thread_index) : m_thread_index(thread_index) {}

    void Entry()
    {
        // taas login.
        std::string err_msg;
        scoped_ptr<common::CredentialGenerator>
            generator(taas::ClientUtility::Login(FLAGS_login_identity, FLAGS_login_role, &err_msg));
        if (generator.get() == NULL) {
            LOG(ERROR) << "Failed to get generator with identity: " << FLAGS_login_identity
                       << ", Error: " << err_msg;
            return;
        }

        TrueRandom random_value;
        int32_t sleep_time = random_value.NextUInt32() % 1000;
        LOG(INFO) << "sleep " << sleep_time << " ms.";
        ThisThread::Sleep(sleep_time);

        zhandle_t* zh;
        zh = zookeeper_init(FLAGS_host.c_str(),
                            GlobalWatcher,
                            3000,
                            0,
                            &m_sync_event,
                            0,
                            generator.get());
        if (zh == NULL) {
            LOG(ERROR) << "null handle, thread_index: " << m_thread_index;
            return;
        }
        if (!m_sync_event.Wait(3000)) {
            LOG(ERROR) << "connect failure, thread_index: " << m_thread_index;
            zookeeper_close(zh);
            return;
        }
        if (ZOO_CONNECTED_STATE == zoo_state(zh)) {
            LOG(INFO) << "connect success. thread_index: " << m_thread_index;
        }

        char buffer[1024];
        int bufflen = sizeof(buffer);
        char path[] = "/zookeeper";
        Stat stat;
        int rc = zoo_get(zh, path, 0, buffer, &bufflen, &stat);
        if (rc != ZOK) {
            LOG(ERROR) << "get path: " << path << " failure. error code: " << rc;
        }
        // zookeeper_close will release a zk quota, so sleep enough time to cause NoQuota error.
        ThisThread::Sleep(5000);
        zookeeper_close(zh);
        LOG(INFO) << "thread_index: " << m_thread_index << " close.";
    }

    int m_thread_index;
    AutoResetEvent m_sync_event;
};

TEST(QuotaTest, QuotaThreadTest)
{
    scoped_array<BaseThread*>threads(new BaseThread*[FLAGS_thread_num]);
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        threads[i] = new QuotaTestThread(i);
    }
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        threads[i]->Start();
    }
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        threads[i]->Join();
    }
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        delete threads[i];
    }
}
