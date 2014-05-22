// Copyright 2012, Tencent Inc.
// Author: Mavis Luo <mavisluo@tencent.com>
//
// TNS api for sdk.

#ifndef COMMON_ZOOKEEPER_TNS_TNS_H
#define COMMON_ZOOKEEPER_TNS_TNS_H

#include <string>
#include <vector>

#include "common/base/closure.h"
#include "common/base/scoped_ptr.h"
#include "common/system/concurrency/thread_pool.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "thirdparty/gtest/gtest.h"

#include "common/zookeeper/tns/tns.pb.h"

namespace common { // namespace common
namespace tns {

class NamedPort;
class TaskTNS;
class JobTNS;

class TNS {
public:
    typedef Closure<void, std::string, int> WatcherCallback;
    typedef Closure<void, std::string, int, std::vector<std::string> > ChildrenWatcherCallback;

    explicit TNS(bool use_proxy = true);
    virtual ~TNS();

    /// @brief Get TNS info of a task.
    ///        The task's name is composed of cluster, role, job name, and task_id.
    ///        Some process may need to access mutipul clusters,
    ///        so cluster is the input para of each TNS api.
    /// @param cluster cluster name of your job, e.g. tjd1-datamining.
    /// @param role the role of your job, e.g. twse_snapshot.
    /// @param job the job name.
    /// @param task_id task id. e.g. 0.
    /// @param task_tns tns info of the task.
    /// @param stat output node stat.
    /// @param watcher Watcher callback.
    ///                call back function should be defined as:
    ///                void CallbackFunction(std::string path, int event);
    /// @retval bool If some error happens, return false, else return true.
    virtual bool GetTaskHostPort(
            const std::string& cluster,
            const std::string& role,
            const std::string& job,
            int task_id,
            TaskTNS* task_tns,
            Stat* stat = NULL,
            WatcherCallback* watcher = NULL);

    /// @brief Get TNS info of a job.
    ///        the name of a job is composed of cluster, role, and job name.
    /// @param cluster cluster name of your job, e.g. tjd1-datamining.
    /// @param role the role of your job, e.g. twse_snapshot.
    /// @param job the job name.
    /// @param task_id task id. e.g. 0.
    /// @param task_tns tns info of the task.
    /// @param stat output node stat.
    /// @param watcher Watcher callback.
    ///                call back function should be defined as:
    ///                void CallbackFunction(
    ///                        std::string path,
    ///                        int event,
    ///                        std::vector<std::string> old_children);
    /// @retval bool If some error happens, return false, else return true.
    virtual bool GetJobHostPort(
            const std::string& cluster,
            const std::string& role,
            const std::string& job,
            JobTNS* job_tns,
            ChildrenWatcherCallback* watcher = NULL);

private:
    friend class TNSTest;

    FRIEND_TEST(TNSTest, GetJobPath);
    FRIEND_TEST(TNSTest, GetTaskPath);
    FRIEND_TEST(TNSTest, GetPrefixFromCluster);
    FRIEND_TEST(TNSTest, GetTaskHostPortFromStr);
    FRIEND_TEST(TNSTest, GetTaskIdFromPath);

    // Get zk path of a job.
    static bool GetJobPath(const std::string& cluster,
                           const std::string& role,
                           const std::string& job,
                           std::string* path);

    // Get zk path of a task.
    static bool GetTaskPath(const std::string& cluster,
                            const std::string& role,
                            const std::string& job,
                            int task_id,
                            std::string* path);

    // Get prefix from a cluster.
    // If the cluster is tjd1-datamining,
    // the prefix is /zk/tjd1/tborg/tjd1-datamining
    static bool GetPrefixFromCluster(const std::string& cluster,
                                     std::string* prefix);

    // Get Task TNS info from the value of the task zk node.
    static bool GetTaskHostPortFromStr(const std::string& tns_str,
                                       TaskTNS* task_tns);

    static std::string GetTaskIdFromPath(const std::string& task_path);

private:
    scoped_ptr<ZookeeperClient> m_zk;

    // If users don't need to watch tns node, it's better to use zk proxy.
    bool m_use_proxy;
};

}  // namespace tns
}  // namespace common

#endif  // COMMON_ZOOKEEPER_TNS_TNS_H
