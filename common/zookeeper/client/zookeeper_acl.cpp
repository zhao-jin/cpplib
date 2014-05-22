// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author: Yubing Yin <yubingyin@tencent.com>
//         Xiaokang Liu <hsiaokangliu@tencent.com>

#include "common/zookeeper/client/zookeeper_acl.h"

#include "thirdparty/glog/logging.h"

namespace common {

std::string PermToString(int32_t perm)
{
    std::string result;
    result.append((perm & kZKPermAdmin) != 0  ? "a" : "-");
    result.append((perm & kZKPermCreate) != 0 ? "c" : "-");
    result.append((perm & kZKPermDelete) != 0 ? "d" : "-");
    result.append((perm & kZKPermWrite) != 0  ? "w" : "-");
    result.append((perm & kZKPermRead) != 0   ? "r" : "-");
    return result;
}

void ZookeeperAcl::AddEntry(const std::string& scheme, const std::string& id, int32_t permission)
{
    for (size_t i = 0; i < m_acls.size(); ++i) {
        ZookeeperAclEntry& entry = m_acls[i];
        if (entry.scheme == scheme && entry.id == id) {
            entry.permission = permission;
            return;
        }
    }
    ZookeeperAclEntry entry = {scheme, id, permission};
    m_acls.push_back(entry);
}

void ZookeeperAcl::RemoveEntry(const std::string& scheme, const std::string& id)
{
    for (size_t i = 0; i < m_acls.size(); ++i) {
        const ZookeeperAclEntry& entry = m_acls[i];
        if (entry.scheme == scheme && entry.id == id) {
            m_acls.erase(m_acls.begin() + i);
            return;
        }
    }
}

bool ZookeeperAcl::GetPermission(const std::string& scheme,
                                 const std::string& id,
                                 int32_t* permission) const
{
    for (size_t i = 0; i < m_acls.size(); ++i) {
        const ZookeeperAclEntry& entry = m_acls[i];
        if (entry.scheme == scheme && entry.id == id) {
            *permission = entry.permission;
            return true;
        }
    }
    return false;
}

} // namespace common

