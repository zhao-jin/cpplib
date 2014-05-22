// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/address_resolver/zk_path_resolver.h"

#include "common/base/scoped_ptr.h"
#include "common/base/string/algorithm.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "poppy/address_resolver/ip_list_resolver.h"

namespace poppy {

void ZkAddressResolver::OnNodeChanged(std::string path, int event)
{
    std::vector<std::string> addresses;
    path = m_zk_cluster + path;
    path = kZookeeperPrefix + path;
    if (m_callback && ResolveAddress(path, &addresses)) {
        m_callback->Run(path, addresses);
    } else {
        LOG(WARNING) << "Address changed but no notification: " << path;
    }
}

bool ZkAddressResolver::ResolveAddress(const std::string& path,
                                       std::vector<std::string>* addresses)
{
    addresses->clear();
    std::string zk_path = StringRemovePrefix(path, kZookeeperPrefix);
    int error_code;
    scoped_ptr<common::ZookeeperNode> node(m_client->OpenWithFullPath(zk_path,
                                                                      &error_code));
    if (node == NULL) {
        LOG(WARNING) << "Failed to open zookeeper node: " << path
            << ", error_code: " << error_code
            << " ["
            << common::ZookeeperErrorString(static_cast<common::ZookeeperErrorCode>(error_code))
            << "]";
        return false;
    }
    std::string::size_type pos = zk_path.find("/");
    if (pos == std::string::npos) {
        return false;
    }
    m_zk_cluster = zk_path.substr(0, pos);
    MutexLocker locker(m_mutex);
    std::string content;
    common::ZookeeperNode::WatcherCallback* closure =
        NewClosure(this, &ZkAddressResolver::OnNodeChanged);
    error_code = node->GetContent(&content, NULL, closure);
    if (error_code == common::ZK_NONODE) {
        error_code = node->Exists(closure);
        if (error_code != common::ZK_NONODE && error_code != common::ZK_OK) {
            LOG(WARNING) << "Failed to add watcher on a non-exist node: " << path
                << ", error_code: " << error_code
                << " ["
                << common::ZookeeperErrorString(static_cast<common::ZookeeperErrorCode>(error_code))
                << "]";
            delete closure;
            return false;
        }
        return true;
    } else if (error_code != common::ZK_OK) {
        LOG(WARNING) << "Failed to get node content: " << path
            << ", error_code: " << error_code
            << " ["
            << common::ZookeeperErrorString(static_cast<common::ZookeeperErrorCode>(error_code))
            << "]";
        delete closure;
        return false;
    }
    return IPAddressResolver::ResolveIPAddress(content, addresses);
}

} // namespace poppy
