// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author: Yubing Yin(yubingyin@tencent.com)

#include "common/zookeeper/sdk/zookeeper.h"

namespace common
{

void ZooKeeperAclVector::SetAcl(const std::string& scheme, const std::string& id,
                                const ZooKeeperAcl& acl)
{
    ZooKeeperId zk_id = { scheme, id };
    m_acls[zk_id] = acl;
}

void ZooKeeperAclVector::SetAcl(const std::string& scheme, const std::string& id,
                                int32_t permissions)
{
    SetAcl(scheme, id, ZooKeeperAcl(permissions));
}

void ZooKeeperAclVector::RemoveAcl(const std::string& scheme, const std::string& id)
{
    ZooKeeperId zk_id = { scheme, id };
    m_acls.erase(zk_id);
}

bool ZooKeeperAclVector::GetAcl(const std::string& scheme, const std::string& id,
                                ZooKeeperAcl* acl) const
{
    ZooKeeperId zk_id = { scheme, id };
    AclMap::const_iterator it = m_acls.find(zk_id);
    if (it == m_acls.end()) {
        return false;
    }
    *acl = it->second;
    return true;
}

void ZooKeeperAclVector::Dump(std::vector<ZooKeeperAclEntry>* acls) const
{
    acls->clear();
    AclMap::const_iterator it = m_acls.begin();
    for (; it != m_acls.end(); ++it) {
        ZooKeeperAclEntry entry = { it->first, it->second.GetPermissions() };
        acls->push_back(entry);
    }
}

} // namespace common
