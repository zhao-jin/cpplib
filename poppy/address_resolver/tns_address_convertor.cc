// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "poppy/address_resolver/tns_address_convertor.h"

#include <vector>
#include "common/base/string/algorithm.h"
#include "common/base/string/concat.h"
#include "common/base/string/string_number.h"
#include "common/system/net/socket.h"
#include "thirdparty/glog/logging.h"

namespace poppy {

static const size_t kTnsJobPathLength = 7;
static const size_t kTnsTaskPathLength = 8;

bool ConvertTnsPathToZkPath(const std::string& address, std::string* path)
{
    path->clear();
    std::vector<std::string> sections;
    SplitString(address, "/", &sections);
    // format: /tns/zk_cluster-tborg_cluster/role/job[/task]
    if (sections.size() < 4u || sections.size() > 5u) {
        return false;
    }
    std::string::size_type pos = sections[1].find("-");
    if (pos == 0 || pos == std::string::npos) {
        return false;
    }
    std::string zk_cluster = sections[1].substr(0, pos);
    StringAppend(path, "/zk/", zk_cluster, "/tborg/", sections[1],
                 "/tns/", sections[2], "/", sections[3]);
    if (sections.size() == 5) {
        StringAppend(path, "/", sections[4]);
    }
    return true;
}

bool ConvertZkPathToTnsPath(const std::string& address, std::string* path)
{
    path->clear();
    std::vector<std::string> sections;
    SplitString(address, "/", &sections);
    // format: /zk/zk_cluster/tborg/zk_cluster-tborg_cluster/tns/role/job[/task]
    if (sections.size() < kTnsJobPathLength || sections.size() > kTnsTaskPathLength) {
        LOG(WARNING) << "Invalid section size: " << sections.size();
        return false;
    }
    StringAppend(path, "/tns/", sections[3], "/", sections[5], "/", sections[6]);
    if (sections.size() == kTnsTaskPathLength) {
        StringAppend(path, "/", sections[7]);
    }
    return true;
}

// task_info format:
// ip=$ip1\nports=port1;port2;...;portN\n$name=$port_name
bool GetTaskAddressFromInfo(const std::string& task_info,
                            const std::string& task_port,
                            std::string* address)
{
    address->clear();
    // Task exists, but no ip port specified.
    if (task_info.empty()) {
        return true;
    }
    std::vector<std::string> vec;
    SplitString(task_info, "\n", &vec);
    if (vec.size() < 2u) {
        // Task is waiting, no valid ip port
        if (StringStartsWith(task_info, "ip=255.255.255.255")) {
            return true;
        }
        LOG(WARNING) << "task info is not valid: " << task_info;
        return false;
    }
    if (vec[0].find("ip=") != 0) {
        LOG(WARNING) << "task info is not valid: " << task_info;
        return false;
    }
    std::string ip_address;
    std::string ip = vec[0].substr(strlen("ip="));
    if (task_port.empty()) {
        // No port name sepecified. use the first port of anonymous port list.
        if (vec[1].find("ports=") != 0) {
            LOG(WARNING) << "task info is not valid: " << task_info;
            return false;
        }
        std::string port_list = vec[1].substr(strlen("ports="));
        vec.clear();
        SplitString(port_list, ";", &vec);
        if (vec.empty()) {
            LOG(WARNING) << "task info is not valid: " << task_info;
            return false;
        }
        ip_address = ip + ":" + vec[0];
    } else {
        size_t port_index = 0;
        if (StringToNumber(task_port, &port_index)) {
            // port=number, use the sepecified port index.
            if (vec[1].find("ports=") != 0) {
                LOG(WARNING) << "task info is not valid: " << task_info;
                return false;
            }
            std::string port_list = vec[1].substr(strlen("ports="));
            vec.clear();
            SplitString(port_list, ";", &vec);
            if (vec.size() <= port_index) {
                LOG(WARNING) << "invalid port list: " << port_list
                    << " and port index: " << port_index;
                return false;
            }
            ip_address = ip + ":" + vec[port_index];
        } else {
            for (size_t i = 1; i < vec.size(); ++i) {
                if (StringStartsWith(vec[i], task_port + "=")) {
                    ip_address = ip + ":" + vec[i].substr(task_port.size() + 1);
                    break;
                }
            }
        }
    }
    SocketAddressInet sock;
    if (sock.Parse(ip_address)) {
        *address = sock.ToString();
        return true;
    }
    return false;
}

} // namespace poppy
