// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/address_resolver/tns_job_resolver.h"

#include <algorithm>
#include <string>
#include <vector>
#include "common/base/scoped_ptr.h"
#include "common/base/string/algorithm.h"
#include "common/base/string/string_number.h"
#include "common/system/net/socket.h"
#include "common/zookeeper/client/zookeeper_node.h"
#include "poppy/address_resolver/tns_address_convertor.h"

namespace poppy {

void TnsJobAddressResolver::OnTaskDataChanged(std::string path, int event)
{
    // Only process node data changed event.
    if (event != common::ZK_NODE_CHANGED_EVENT || !m_callback) {
        return;
    }
    std::string::size_type pos = path.rfind("/");
    if (pos == std::string::npos) {
        LOG(WARNING) << "Invalid task path: " << path;
        return;
    }
    std::string job_path = path.substr(0, pos);
    std::string port;
    {
        MutexLocker locker(m_mutex);
        std::map<std::string, std::string>::iterator iter = m_job_port.find(job_path);
        if (iter == m_job_port.end()) {
            LOG(WARNING) << "No job port: " << path;
            return;
        }
        port = iter->second;
    }
    std::string tns_path;
    if (!ConvertZkPathToTnsPath(job_path, &tns_path)) {
        LOG(WARNING) << "Failed to convert zk path to tns path: " << path;
        return;
    }
    if (!port.empty()) {
        tns_path += "?port=" + port;
    }

    std::vector<std::string> addresses;
    std::vector<std::string> children;
    scoped_ptr<common::ZookeeperNode> node(m_client->Open(job_path));
    if (node == NULL) {
        LOG(WARNING) << "Failed to open node: " << job_path;
        return;
    }
    int ret = node->GetChildren(&children);
    if (ret == common::ZK_NONODE) {
        m_callback->Run(tns_path, addresses);
        return;
    } else if (ret != common::ZK_OK) {
        LOG(WARNING) << "Failed to get children: " << job_path << ", return: " << ret;
        return;
    }

    std::string value;
    std::string ip_address;
    for (size_t i = 0; i < children.size(); ++i) {
        const std::string& child_path = children[i];
        scoped_ptr<common::ZookeeperNode> child(m_client->Open(child_path));
        if (child == NULL) {
            LOG(WARNING) << "Failed to open node: " << child_path;
            return;
        }
        if (child_path == path) {
            Closure<void, std::string, int>* callback =
                NewClosure(this, &TnsJobAddressResolver::OnTaskDataChanged);
            ret = child->GetContent(&value, NULL, callback);
            if (ret == common::ZK_NONODE) {
                delete callback;
                continue;
            } else if (ret != common::ZK_OK) {
                LOG(WARNING) << "Failed to get data from node: " << child_path;
                delete callback;
                return;
            }
        } else {
            ret = child->GetContent(&value);
            if (ret == common::ZK_NONODE) {
                continue;
            } else if (ret != common::ZK_OK) {
                LOG(WARNING) << "Failed to get data from node: " << child_path;
                return;
            }
        }
        if (!GetTaskAddressFromInfo(value, port, &ip_address)) {
            return;
        }
        if (!ip_address.empty()) {
            addresses.push_back(ip_address);
        }
    }
    m_callback->Run(tns_path, addresses);
}

void TnsJobAddressResolver::OnTaskListChanged(std::string path,
                                              int event,
                                              std::vector<std::string> v)
{
    scoped_ptr<common::ZookeeperNode> node(m_client->Open(path));
    if (node == NULL) {
        LOG(WARNING) << "Failed to open node: " << path;
        return;
    }

    std::string port;
    {
        MutexLocker locker(m_mutex);
        std::map<std::string, std::string>::iterator iter = m_job_port.find(path);
        if (iter == m_job_port.end()) {
            LOG(WARNING) << "No job port: " << path;
            return;
        }
        port = iter->second;
    }
    std::string tns_path;
    if (!ConvertZkPathToTnsPath(path, &tns_path)) {
        LOG(WARNING) << "Failed to convert zk path to tns path: " << path;
        return;
    }
    if (!port.empty()) {
        tns_path += "?port=" + port;
    }

    std::vector<std::string> addresses, children;
    Closure<void, std::string, int, std::vector<std::string> >* closure =
        NewClosure(this, &TnsJobAddressResolver::OnTaskListChanged);
    int ret = node->GetChildren(&children, closure);
    if (ret == common::ZK_NONODE) {
        delete closure;
        Closure<void, std::string, int>* callback =
            NewClosure(this, &TnsJobAddressResolver::OnJobNodeCreated);
        int ret = node->Exists(callback);
        if (ret != common::ZK_OK && ret != common::ZK_NONODE) {
            LOG(WARNING) << "Failed add watcher on node: " << path;
            delete closure;
            return;
        }
        if (m_callback) {
            m_callback->Run(tns_path, addresses);
        }
        return;
    } else if (ret != common::ZK_OK) {
        LOG(WARNING) << "Failed to get children: " << path << ", return: " << ret;
        delete closure;
        return;
    }
    std::string value;
    std::string ip_address;
    for (size_t i = 0; i < children.size(); ++i) {
        const std::string& child_path = children[i];
        scoped_ptr<common::ZookeeperNode> child(m_client->Open(child_path));
        if (child == NULL) {
            LOG(WARNING) << "Failed to open node: " << child_path;
            return;
        }
        if (std::find(v.begin(), v.end(), child_path) == v.end()) {
            Closure<void, std::string, int>* callback =
                NewClosure(this, &TnsJobAddressResolver::OnTaskDataChanged);
            ret = child->GetContent(&value, NULL, callback);
            if (ret == common::ZK_NONODE) {
                delete callback;
                continue;
            } else if (ret != common::ZK_OK) {
                LOG(WARNING) << "Failed to get data from node: " << child_path;
                delete callback;
                return;
            }
        } else {
            ret = child->GetContent(&value);
            if (ret == common::ZK_NONODE) {
                continue;
            } else if (ret != common::ZK_OK) {
                LOG(WARNING) << "Failed to get data from node: " << child_path;
                return;
            }
        }
        if (!GetTaskAddressFromInfo(value, port, &ip_address)) {
            return;
        }
        if (!ip_address.empty()) {
            addresses.push_back(ip_address);
        }
    }
    if (m_callback) {
        m_callback->Run(tns_path, addresses);
    }
}

void TnsJobAddressResolver::OnJobNodeCreated(std::string path, int event)
{
    std::string port;
    {
        MutexLocker locker(m_mutex);
        std::map<std::string, std::string>::iterator iter = m_job_port.find(path);
        if (iter == m_job_port.end()) {
            LOG(WARNING) << "No job port: " << path;
            return;
        }
        port = iter->second;
    }
    std::string tns_path;
    if (!ConvertZkPathToTnsPath(path, &tns_path)) {
        LOG(WARNING) << "Failed to convert zk path to tns path: " << path;
        return;
    }
    if (!port.empty()) {
        tns_path += "?port=" + port;
    }

    std::vector<std::string> addresses;
    if (m_callback && ResolveAddress(tns_path, &addresses)) {
        m_callback->Run(tns_path, addresses);
    }
}

bool TnsJobAddressResolver::ResolveAddress(const std::string& path,
                                        std::vector<std::string>* addresses)
{
    addresses->clear();
    std::string port = "";
    std::string tns_path = path;
    std::string::size_type pos = path.find("?");
    if (pos != std::string::npos) {
        if (!StringStartsWith(path.substr(pos), "?port=")) {
            return false;
        }
        tns_path = path.substr(0, pos);
        port = path.substr(pos + strlen("?port="));
    }

    std::string zk_path;
    if (!ConvertTnsPathToZkPath(tns_path, &zk_path)) {
        LOG(WARNING) << "Failed to convert tns path to zk path: " << path;
    }
    scoped_ptr<common::ZookeeperNode> node(m_client->Open(zk_path));
    if (node == NULL) {
        LOG(WARNING) << "Failed to open job node: " << path;
        return false;
    }

    MutexLocker locker(m_mutex);

    std::vector<std::string> children;
    Closure<void, std::string, int, std::vector<std::string> >* closure =
        NewClosure(this, &TnsJobAddressResolver::OnTaskListChanged);
    int ret = node->GetChildren(&children, closure);
    if (ret == common::ZK_NONODE) {
        LOG(WARNING) << "No node: " << zk_path;
        delete closure;
        Closure<void, std::string, int>* callback =
            NewClosure(this, &TnsJobAddressResolver::OnJobNodeCreated);
        int ret = node->Exists(callback);
        if (ret != common::ZK_OK && ret != common::ZK_NONODE) {
            LOG(WARNING) << "Failed add watcher on node: " << path;
            delete closure;
            return false;
        }
        m_job_port[zk_path] = port;
        return true;
    } else if (ret != common::ZK_OK) {
        LOG(WARNING) << "Failed to get children: " << zk_path << ", return: " << ret;
        delete closure;
        return false;
    }

    std::string value;
    std::string ip_address;
    for (size_t i = 0; i < children.size(); ++i) {
        const std::string& child_path = children[i];
        scoped_ptr<common::ZookeeperNode> child(m_client->Open(child_path));
        Closure<void, std::string, int>* callback =
            NewClosure(this, &TnsJobAddressResolver::OnTaskDataChanged);
        ret = child->GetContent(&value, NULL, callback);
        if (ret == common::ZK_NONODE) {
            delete callback;
            continue;
        } else if (ret != common::ZK_OK) {
            LOG(WARNING) << "Failed to get data from node: " << child_path;
            delete callback;
            return false;
        }
        if (!GetTaskAddressFromInfo(value, port, &ip_address)) {
            return false;
        }
        if (!ip_address.empty()) {
            addresses->push_back(ip_address);
        }
    }
    m_job_port[zk_path] = port;
    return true;
}

} // namespace poppy
