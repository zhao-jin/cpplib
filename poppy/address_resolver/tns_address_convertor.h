// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef POPPY_ADDRESS_RESOLVER_TNS_ADDRESS_CONVERTOR_H
#define POPPY_ADDRESS_RESOLVER_TNS_ADDRESS_CONVERTOR_H

#include <string>

namespace poppy {

/// @brief Get tborg task address from tborg task info.
/// @param task_info task info in zk.
/// @param task_port task port name or port index
/// @param address output task socket address
/// @retval true if parse info successfully.
bool GetTaskAddressFromInfo(const std::string& task_info,
                            const std::string& task_port,
                            std::string* address);

/// @brief Convert a tns path to real zookeeper path.
/// @param address tns path, format /tns/zk_cluster-tborg_cluster/role/jobname[/taskid]
/// @param path output zookeeper path
/// @retval true if success
bool ConvertTnsPathToZkPath(const std::string& address, std::string* path);

/// @brief Convert a zookeeper path to tns path.
/// @param address zookeeper address
/// @param path output tns path
/// @retval true if success
bool ConvertZkPathToTnsPath(const std::string& address, std::string* path);

} // namespace poppy

#endif // POPPY_ADDRESS_RESOLVER_TNS_ADDRESS_CONVERTOR_H
