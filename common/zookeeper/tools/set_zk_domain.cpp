// Copyright (c) 2012, Tencent Inc.
// All rights reserved.
//
// Author: rabbitliu <rabbitliu@tencent.com>
// Created: 06/18/12
// Description:

#include <fstream>
#include <vector>
#include "common/base/scoped_ptr.h"
#include "common/base/string/algorithm.h"
#include "common/base/string/string_number.h"
#include "common/system/net/domain_resolver.h"
#include "common/zookeeper/client/zookeeper_client.h"
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

DEFINE_string(root_zk_host, "tf.zk.oa.com:2181", "root zookeeper host.");
DEFINE_string(zk_hosts,
              "tjzt,"
              "tjnl,"
              "tjd1,"
              "tjd2,"
              "tjd1sv,"
              "tjd2sv,"
              "xaec,"
              "xajksv,"
              "shpcsv,ci,"
              "cdgx,"
              "szsk,"
              "szsksv,"
              "shonsv,"
              "tj.proxy,"
              "xa.proxy,"
              "sh.proxy,"
              "ci.proxy,"
              "cd.proxy,"
              "sz.proxy",
              "zk_host to set");
DEFINE_string(zk_hosts_file, "", "zk_hosts to set");
DEFINE_string(path, "/zk/tf/router/test_hosts", "zk path to set hosts");

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    std::string all_hosts_contents;
    // no file, use domain resolver.
    if (FLAGS_zk_hosts_file == "") {
        LOG(INFO) << "Use domain resolver to get zookeeper clusters ip: FLAGS_zk_hosts";
        std::vector<std::string> all_hosts;
        SplitString(FLAGS_zk_hosts, ",", &all_hosts);
        std::vector<std::string>::iterator it = all_hosts.begin();
        for (; it != all_hosts.end(); ++it) {
            std::string zk_host = *it + ".zk.oa.com";
            std::vector<IPAddress> addresses;
            if (!common::DomainResolver::ResolveIpAddress(zk_host, &addresses)) {
                LOG(ERROR) << "Resolve ip address failure, zk_host: " << zk_host;
                return EXIT_FAILURE;
            }
            all_hosts_contents += zk_host + ":2181=";
            size_t i = 0;
            for (; i < addresses.size() - 1; ++i) {
                all_hosts_contents += addresses[i].ToString() + ":2181,";
            }
            all_hosts_contents += addresses[i].ToString() + ":2181";
            all_hosts_contents += "\n";
        }
        all_hosts_contents += "dns_failure.zk.oa.com:2181=127.0.0.1:2181\n"; // used for dns test.
    } else {
        LOG(INFO) << "Loading zookeeper clusters ip from file: " << FLAGS_zk_hosts_file;
        std::ifstream zk_hosts_file(FLAGS_zk_hosts_file.c_str());
        if (!zk_hosts_file) {
            LOG(ERROR) << "Read file failure, filename: " << FLAGS_zk_hosts_file;
            return EXIT_FAILURE;
        }
        std::string line;
        while (getline(zk_hosts_file, line)) {
            if (line[0] != '[') {
                continue;
            }
            all_hosts_contents += line.substr(1, line.size() - 2) + ".zk.oa.com:2181=";
            if (getline(zk_hosts_file, line)) {
                size_t index = line.find("=");
                if (index == std::string::npos) {
                    LOG(ERROR) << "File no hosts size.";
                    zk_hosts_file.close();
                    return EXIT_FAILURE;
                }
                int size = 0;
                if (!StringToNumber(line.substr(index + 1), &size)) {
                    LOG(ERROR) << "Hosts size not a number.";
                    zk_hosts_file.close();
                    return EXIT_FAILURE;
                }
                int i = 0;
                while (getline(zk_hosts_file, line) && i < size - 1) {
                    all_hosts_contents += line + ",";
                    ++i;
                }
                all_hosts_contents += line + "\n";
            }
        }
        zk_hosts_file.close();
    }
    // The following zk clinet use Options to Login as zkadmin to set /zk/tf/router/zk_hosts
    // common::ZookeeperClient client(
    //     common::ZookeeperClient::Options("zkadmin:Zkadmin2012", false));
    common::ZookeeperClient client;
    scoped_ptr<common::ZookeeperNode> node(client.Open(FLAGS_path));
    if (node == NULL) {
        LOG(ERROR) << "Can not open node: " << FLAGS_path;
        return EXIT_FAILURE;
    }
    int error_code = node->SetContent(all_hosts_contents);
    if (error_code != common::ZK_OK) {
        LOG(ERROR) << "Set contents failure, path: " << FLAGS_path;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
