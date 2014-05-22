// Copyright (c) 2013, Tencent Inc.
// All rights reserved.
//
// Author: rabbitliu <rabbitliu@tencent.com>
// Created: 09/25/13
// Description:

#include <string>

#include "common/base/module.h"
#include "common/base/scoped_ptr.h"
#include "common/base/string/string_number.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

DEFINE_string(set_role, "", "which role need set quota.");
DEFINE_string(cluster, "", "which cluster to set.");
DEFINE_uint64(quota_limits, 0, "how many quota to set for set_role");

DECLARE_string(role);
DECLARE_string(identity);
DECLARE_bool(taas_client_enable);

int main(int argc, char** argv)
{
    FLAGS_role = "zk_admin";
    FLAGS_identity = "rabbitliu";
    FLAGS_taas_client_enable = true;

    InitAllModules(&argc, &argv);

    std::string quota_limits = FLAGS_cluster + ".zk.oa.com:2181/zookeeper/role/" + FLAGS_set_role
        + "/quota_limits";
    std::string quota_stats = FLAGS_cluster + ".zk.oa.com:2181/zookeeper/role/" + FLAGS_set_role
        + "/quota_stats";

    common::ZookeeperClient client;
    scoped_ptr<common::ZookeeperNode> node(client.OpenWithFullPath(quota_limits));
    if (node == NULL) {
        LOG(ERROR) << "Open node failed for path: " << quota_limits;
        return EXIT_FAILURE;
    }

    // 重复创建没有问题，对于创建ok和node exist均认为正确
    int ret = node->RecursiveCreate();
    if (ret != common::ZK_OK && ret != common::ZK_NODEEXISTS) {
        LOG(ERROR) << "Create quota_limits node failed for path: " << node->GetPath()
            << ", ret: " << ret;
        return EXIT_FAILURE;
    }
    ret = node->SetContent(NumberToString(FLAGS_quota_limits));
    if (ret != common::ZK_OK && ret != common::ZK_NODEEXISTS) {
        LOG(ERROR) << "Set quota_limits node failed for path: " << node->GetPath()
            << ", content: " << FLAGS_quota_limits << ", ret: " << ret;
        return EXIT_FAILURE;
    }

    node.reset(client.OpenWithFullPath(quota_stats));
    if (node == NULL) {
        LOG(ERROR) << "Open node failed for path: " << quota_stats;
        return EXIT_FAILURE;
    }

    // stat节点存在的是为了修改其role的quota，stat节点的内容不变
    if (node->Exists() == common::ZK_OK || node->Exists() == common::ZK_NODEEXISTS) {
        LOG(INFO) << "Set quota success for role: " << FLAGS_set_role
            << "quota: " << FLAGS_quota_limits;
        return EXIT_SUCCESS;
    }
    // stat node不存在
    ret = node->RecursiveCreate();
    if (ret != common::ZK_OK) {
        LOG(ERROR) << "Create quota stats node failed for path: " << node->GetPath()
            << ", ret: " << ret;
        return EXIT_FAILURE;
    }
    // 重新设置它的内容为0
    ret = node->SetContent("0");
    if (ret != common::ZK_OK) {
        LOG(ERROR) << "Set stats node failed for path: " << node->GetPath()
            << ", ret: " << ret;
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Set quota success for role: " << FLAGS_set_role
            << "quota: " << FLAGS_quota_limits;
    return EXIT_SUCCESS;
}
