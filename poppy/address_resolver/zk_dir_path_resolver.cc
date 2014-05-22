// Copyright (c) 2013, Tencent Inc.
// All rights reserved.
//
// Author: Simon Wang <simonwang@tencent.com>
// Created: 03/15/13
// Description:

#include "poppy/address_resolver/zk_dir_path_resolver.h"

#include "common/base/scoped_ptr.h"
#include "common/base/string/algorithm.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "poppy/address_resolver/ip_list_resolver.h"

namespace poppy {

void ZkDirAddressResolver::OnChildrenChanged(std::string path,
                                             int event,
                                             std::vector<std::string> old_children)
{
    std::vector<std::string> addresses;
    SplitString(path, "/", &addresses);
    if (addresses.size() < 2u) {
        LOG(WARNING) << "Invalid watched path: " << path;
        return;
    }
    path = addresses[1] + ".zk.oa.com:2181" + path;
    path = kZookeeperDirPrefix + path;
    if (m_callback && ResolveAddress(path, &addresses)) {
        m_callback->Run(path, addresses);
    } else {
        LOG(WARNING) << "Address changed but no notification: " << path;
    }
}

bool ZkDirAddressResolver::ResolveAddress(const std::string& path,
                                          std::vector<std::string>* addresses)
{
    addresses->clear();
    std::string zk_path = StringRemovePrefix(path, kZookeeperDirPrefix);
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
    MutexLocker locker(m_mutex);
    std::vector<std::string> new_children;
    common::ZookeeperNode::ChildrenWatcherCallback* closure =
        NewClosure(this, &ZkDirAddressResolver::OnChildrenChanged);
    error_code = node->GetChildren(&new_children, closure);
    if (error_code != common::ZK_OK) {
        LOG(WARNING) << "Failed to get children: " << zk_path
            << ", error_code: " << error_code
            << " ["
            << common::ZookeeperErrorString(static_cast<common::ZookeeperErrorCode>(error_code))
            << "]";
        delete closure;
        return false;
    }
    std::string content;
    scoped_ptr<common::ZookeeperNode> children_node;
    for (std::vector<std::string>::const_iterator iter = new_children.begin();
         iter != new_children.end();
         ++iter) {
        children_node.reset(m_client->Open(*iter));
        if (children_node == NULL) {
            LOG(WARNING) << "Failed to open zookeeper node: " << *iter;
            continue;
        }
        std::string children_content;
        error_code = children_node->GetContent(&children_content);
        if (error_code != common::ZK_OK) {
            LOG(WARNING) << "Failed to get node content: " << *iter
                << ", error_code: " << error_code
                << " ["
                << common::ZookeeperErrorString(static_cast<common::ZookeeperErrorCode>(error_code))
                << "]";
            continue;
        }
        content += children_content;
        content += ",";
    }
    if (content.empty()) {
        LOG(WARNING) << "Failed to get children content for " << path;
        return false;
    }
    content = content.substr(0, content.size() - 1);
    return IPAddressResolver::ResolveIPAddress(content, addresses);
}

} // namespace poppy

