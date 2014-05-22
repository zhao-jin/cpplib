// Copyright 2012, Tencent Inc.
// Author: Mavis Luo <mavisluo@tencent.com>

#include "common/zookeeper/tns/tns.h"

#include <string>
#include <vector>

#include "common/base/string/algorithm.h"
#include "common/base/string/concat.h"
#include "common/base/string/string_number.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "common/zookeeper/tns/tns.pb.h"
#include "thirdparty/glog/logging.h"

namespace common {
namespace tns {

TNS::TNS(bool use_proxy) : m_use_proxy(use_proxy) {
    if (use_proxy) {
        // readonly.
        m_zk.reset(new ZookeeperClient(
                ZookeeperClient::Options("", true)));
    } else {
        m_zk.reset(new ZookeeperClient());
    }
}

TNS::~TNS() {}

bool TNS::GetTaskHostPort(const std::string& cluster,
                          const std::string& role,
                          const std::string& job,
                          int task_id,
                          TaskTNS* task_tns,
                          Stat* stat,
                          WatcherCallback* watcher) {
    // Check the param.
    if (cluster.empty() || role.empty() || job.empty()
        || task_id < 0) {
        return false;
    }
    // Zookeeper proxy can't support watcher.
    if (watcher && m_use_proxy) {
        LOG(ERROR) << "Zookeeper proxy can't support watcher temporarily";
        return false;
    }
    task_tns->Clear();

    // Get task's zk path.
    std::string task_path;
    if (!GetTaskPath(cluster, role, job, task_id, &task_path)) {
        return false;
    }

    int error_code = ZK_OK;
    scoped_ptr<ZookeeperNode> node(m_zk->Open(task_path, &error_code));
    if (node == NULL) {
        LOG(ERROR) << "Open task path [" << task_path << "] failed. Error code: " << error_code
            << " [" << ZookeeperErrorString(static_cast<ZookeeperErrorCode>(error_code)) << "]";
        return false;
    }
    std::string tns_value;
    int ret = node->GetContent(&tns_value, stat, watcher);
    if (ZK_OK != ret) {
        LOG(INFO) << "Can't get the tns value of " << task_path;
        LOG(INFO) << "The reason is: "
            << ZookeeperErrorString(ZookeeperErrorCode(ret));
        return false;
    }

    if (!GetTaskHostPortFromStr(tns_value, task_tns)) {
        return false;
    }
    task_tns->set_task_id(task_id);

    return true;
}

bool TNS::GetJobHostPort(
        const std::string& cluster,
        const std::string& role,
        const std::string& job,
        JobTNS* job_tns,
        ChildrenWatcherCallback* watcher) {
    if (cluster.empty() || role.empty() || job.empty()) {
        return false;
    }
    if (watcher && m_use_proxy) {
        LOG(ERROR) << "Zookeeper proxy can't support watcher temporarily";
        return false;
    }

    job_tns->Clear();

    std::string job_path;
    if (!GetJobPath(cluster, role, job, &job_path)) {
        return false;
    }

    int error_code = ZK_OK;
    scoped_ptr<ZookeeperNode> node(m_zk->Open(job_path, &error_code));
    if (node == NULL) {
        LOG(ERROR) << "Open job path [" << job_path << "] failed. Error code: " << error_code
            << " [" << ZookeeperErrorString(static_cast<ZookeeperErrorCode>(error_code)) << "]";
        return false;
    }
    // Get All children node path.
    std::vector<std::string> children;
    int ret = node->GetChildren(&children, watcher);
    if (ZK_OK != ret) {
        LOG(ERROR) << "Error happens when get job's children node ["
            << job_path << "]";
        LOG(ERROR) << "The reason is: "
            << ZookeeperErrorString((ZookeeperErrorCode(ret)));
        return false;
    }
    // Get tns of each task.
    for (std::vector<std::string>::const_iterator it = children.begin();
         it != children.end();
         ++it) {
        std::string tns_value;
        int task_id = -1;
        std::string task_id_str = GetTaskIdFromPath(*it);
        StringToNumber(task_id_str, &task_id);
        if (task_id < 0) {
            continue;
        }
        ret = node->GetChildContent(task_id_str, &tns_value);
        if (ZK_OK != ret) {
            LOG(ERROR) << "Error happens when get task's tns node ["
                << *it << "]";
            LOG(ERROR) << "The reason is: "
                << ZookeeperErrorString((ZookeeperErrorCode(ret)));
            continue;
        }
        TaskTNS* task_tns = job_tns->add_task_tns();
        if (!GetTaskHostPortFromStr(tns_value, task_tns)) {
            return false;
        }
        task_tns->set_task_id(task_id);
    }

    return true;
}

bool TNS::GetJobPath(const std::string& cluster,
                     const std::string& role,
                     const std::string& job,
                     std::string* path) {
    CHECK_NOTNULL(path);
    path->clear();

    std::string prefix;
    if (!GetPrefixFromCluster(cluster, &prefix)) {
        return false;
    }

    *path = prefix + "/tns" + "/" + role + "/" + job;
    return true;
}

bool TNS::GetTaskPath(const std::string& cluster,
                      const std::string& role,
                      const std::string& job,
                      int task_id,
                      std::string* path) {
    CHECK_NOTNULL(path);
    path->clear();

    std::string job_path;
    if (!GetJobPath(cluster, role, job, &job_path)) {
        return false;
    }

    *path = StringConcat(job_path, "/", task_id);
    return true;
}

bool TNS::GetPrefixFromCluster(const std::string& cluster,
                               std::string* prefix) {
    CHECK_NOTNULL(prefix);
    prefix->clear();

    std::string::size_type pos = cluster.find("-");

    if (std::string::npos == pos || 0 == pos || cluster.size() - 1 == pos) {
        LOG(ERROR) << "The cluster_name has no '-' or begin with '-' or has"
            "no cluster's self info ["  << cluster << "]";
        return false;
    }

    std::string idc = cluster.substr(0, pos);
    *prefix = "/zk/" + idc + "/tborg/" + cluster;

    return true;
}

bool TNS::GetTaskHostPortFromStr(const std::string& tns_str,
                                 TaskTNS* task_tns) {
    CHECK_NOTNULL(task_tns);
    task_tns->Clear();

    std::vector<std::string> items; // results splited by '/n'
    SplitString(tns_str, "\n", &items);
    if (items.size() != 0) {
        for (std::vector<std::string>::size_type ix = 0;
             ix < items.size();
             ++ix) {
            std::vector<std::string> one_item;
            SplitString(items[ix], "=", &one_item);
            if (one_item.size() < 2) {
                continue;
            }
            std::string lhs = StringTrim(one_item[0]);
            // Analyse the ip info
            if (lhs == "ip") {
                task_tns->set_ip(StringTrim(one_item[1]));
            } else if (lhs == "ports") {
                // Analyse the ports info
                std::vector<std::string> str_ports;
                SplitString(StringTrim(one_item[1]), ";", &str_ports);
                for (std::vector<std::string>::size_type pos = 0;
                     pos < str_ports.size();
                     ++pos) {
                    task_tns->add_ports(atoi(StringTrim(str_ports[pos]).c_str()));
                }
            } else {
                // Named port.
                NamedPort* named_port = task_tns->add_named_ports();
                named_port->set_port_name(lhs);
                named_port->set_port(atoi(StringTrim(one_item[1]).c_str()));
            }
        }
    } else {
        return false;
    }
    return true;
}

std::string TNS::GetTaskIdFromPath(const std::string& task_path) {
    std::string::size_type pos = task_path.rfind("/");
    CHECK(std::string::npos != pos);
    return task_path.substr(pos + 1, task_path.size());
}

}  // namespace tns
}  // namespace common
