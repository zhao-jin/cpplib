// Copyright 2012, Tencent Inc.
// Author: Rui Li <rli@tencent.com>
//         Xiaokang Liu <hsiaokangliu@tencent.com>

#include "common/zookeeper/client/zookeeper_node_mock.h"

namespace common {

ZookeeperNodeMock* CastToZookeeperNodeMock(ZookeeperNode* node,
                                           const char* file,
                                           int line)
{
    ZookeeperNodeMock* node_mock =
        dynamic_cast<ZookeeperNodeMock*>(node); // NOLINT(readability/casting)
    CHECK(node_mock != NULL)
        << file << ":" << line
        << " First parameter of EXPECT_ZK_CALL is not type of ::common::ZookeeperNodeMock&";
    return node_mock;
}

void ZookeeperNodeMock::SetDefaultBehavior()
{
    using ::testing::_;

    EXPECT_CALL(*this, Create(_, _)).WillRepeatedly(::testing::Return(m_default_ret_code));
    EXPECT_CALL(*this, GetContent(_, _, _)).
        WillRepeatedly(::testing::DoAll(::testing::SetArgPointee<0>(kMockContent),
        ::testing::Return(m_default_ret_code)));
    EXPECT_CALL(*this, GetContentAsJson(_)).WillRepeatedly(::testing::Return(m_default_ret_code));

    EXPECT_CALL(*this, SetContent(_, _)).WillRepeatedly(::testing::Return(m_default_ret_code));
    EXPECT_CALL(*this, Exists(_)).WillRepeatedly(::testing::Return(m_default_ret_code));
    EXPECT_CALL(*this, Delete(_)).WillRepeatedly(::testing::Return(m_default_ret_code));
    EXPECT_CALL(*this, RecursiveDelete()).WillRepeatedly(::testing::Return(m_default_ret_code));

    EXPECT_CALL(*this, GetChildContent(_, _, _)).
        WillRepeatedly(::testing::DoAll(::testing::SetArgPointee<1>(kMockContent),
        ::testing::Return(m_default_ret_code)));
    EXPECT_CALL(*this, SetChildContent(_, _, _)).
        WillRepeatedly(::testing::Return(m_default_ret_code));
    EXPECT_CALL(*this, GetChildren(_, _)).WillRepeatedly(::testing::Return(m_default_ret_code));
    EXPECT_CALL(*this, Lock()).WillRepeatedly(::testing::Return(m_default_ret_code));
    EXPECT_CALL(*this, TryLock()).WillRepeatedly(::testing::Return(m_default_ret_code));
    EXPECT_CALL(*this, Unlock()).WillRepeatedly(::testing::Return(m_default_ret_code));
}

} // namespace common
