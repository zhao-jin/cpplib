// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/address_resolver/tns_task_resolver.h"

#include "common/base/scoped_ptr.h"
#include "common/base/string/algorithm.h"
#include "common/zookeeper/client/zookeeper_node.h"
#include "poppy/address_resolver/tns_address_convertor.h"

namespace poppy {

void TnsTaskAddressResolver::OnTaskChanged(std::string path, int event)
{
    std::string tns_path;
    if (!ConvertZkPathToTnsPath(path, &tns_path)) {
        LOG(WARNING) << "Failed to convert zk path to tns path: " << path;
        return;
    }
    std::string port;
    {
        MutexLocker locker(m_mutex);
        std::map<std::string, std::string>::iterator iter = m_task_port.find(path);
        if (iter == m_task_port.end()) {
            LOG(WARNING) << "No task port: " << path;
            return;
        }
        port = iter->second;
    }
    if (!port.empty()) {
        tns_path += "?port=" + port;
    }

    std::vector<std::string> addresses;
    if (m_callback && ResolveAddress(tns_path, &addresses)) {
        m_callback->Run(tns_path, addresses);
    } else {
        LOG(WARNING) << "Address changed but no notification: " << tns_path;
    }
}

bool TnsTaskAddressResolver::ResolveAddress(const std::string& path,
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
        LOG(WARNING) << "Failed to open task node: " << path;
        return false;
    }

    MutexLocker lock(m_mutex);

    std::string content;
    common::ZookeeperNode::WatcherCallback* closure =
        NewClosure(this, &TnsTaskAddressResolver::OnTaskChanged);
    int error_code = node->GetContent(&content, NULL, closure);
    if (error_code == common::ZK_NONODE) {
        LOG(WARNING) << "No node: " << zk_path;
        error_code = node->Exists(closure);
        if (error_code != common::ZK_NONODE && error_code != common::ZK_OK) {
            LOG(WARNING) << "Failed to add watcher on a non-exist node: " << path;
            delete closure;
            return false;
        }
        m_task_port[zk_path] = port;
        return true;
    } else if (error_code != common::ZK_OK) {
        LOG(WARNING) << "Failed to get node content: " << path;
        delete closure;
        return false;
    }
    std::string addr;
    if (!GetTaskAddressFromInfo(content, port, &addr)) {
        return false;
    }
    m_task_port[zk_path] = port;
    addresses->clear();
    if (!addr.empty()) {
        addresses->push_back(addr);
    }
    return true;
}

} // namespace poppy
