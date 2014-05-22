// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author: Yubing Yin <yubingyin@tencent.com>
//         Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_ACL_H
#define COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_ACL_H

#include <string>
#include <vector>
#include "common/base/stdint.h"

namespace common {

enum ZookeeperPermission
{
    kZKPermAdmin  = (1 << 4), // acl admin
    kZKPermDelete = (1 << 3), // delete children
    kZKPermCreate = (1 << 2), // create children
    kZKPermWrite  = (1 << 1), // write
    kZKPermRead   = (1 << 0), // read
    kZKPermAll    = 0x1F,     // all
};

std::string PermToString(int32_t perm);

struct ZookeeperAclEntry
{
    std::string scheme;
    std::string id;
    int32_t permission;
};

class ZookeeperAcl
{
public:
    typedef std::vector<ZookeeperAclEntry>::const_iterator iterator;
    ZookeeperAcl() {}
    void AddEntry(const std::string& scheme, const std::string& id, int32_t permission);
    void RemoveEntry(const std::string& scheme, const std::string& id);
    bool GetPermission(const std::string& scheme, const std::string& id, int32_t* permission) const;
    void Clear()
    {
        m_acls.clear();
    }

    iterator begin() const
    {
        return m_acls.begin();
    }
    iterator end() const
    {
        return m_acls.end();
    }

private:
    std::vector<ZookeeperAclEntry> m_acls;
};

} // namespace common

#endif // COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_ACL_H
