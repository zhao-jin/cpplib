#!/bin/bash
#
# Copyright (c) 2012, Tencent Inc.
#
# Author      : Michaelpeng <michaelpeng@tencent.com>
# Date        : Mar 20, 2012
#
# Description : This script is used to update the conf file on remote machine.
#               We need to update the conf file before starting remote process.


# Check the number of arguments $1- mirror id  $2- remote machine id
if [ $# -lt 2 ]
then
    prog=`basename $0`
    printf "usage:$prog mirror_id remote_id\n"
    exit 1
fi

mirr_id=$1
remote_id=$2
id=$remote_id

source const_ip_mirr_${mirr_id}.sh
total_num_machine=${#ip[@]}
[[ $remote_id -ge $total_num_machine ]] && echo "remote machine id out of bounds" && exit 1

source common_def.sh
source project_def.sh

re_ip=${ip[${id}]}
re_ssh_port=${ssh_port[${id}]}
re_user=${user[${id}]}
re_passwd=${passwd[${id}]}
re_dest_dir=${install_dir[${id}]}

# get the configuration and update to the conf
re_path="${re_dest_dir}/${proj_name}"
re_conf_path="${re_path}/${proj_name}"

[[ ! -d "${proj_name}" ]] && echo "Not having the conf path for proj ${proj_name} " && exit 1

new_conf_file="./${proj_name}/${conf_file_name}"
echo "Config file is ${new_conf_file}"
[[ ! -f ${new_conf_file} ]] && echo "Not having the local conf file to update for ${proj_name} " && exit 1

# update the conf file from const definition file
# It is project specific.
re_target_ip=${ip[${id}]}
re_target_port=${port[${id}]}

echo "Remote ip is $re_target_ip"
set_conf_value ${new_conf_file} 'LOCAL_ADDR' "\"$re_target_ip\""
set_conf_value ${new_conf_file} 'PORT' "\"$re_target_port\""

# overwrite the conf file on remote machine
./rcp.exp $re_ip $re_ssh_port $re_user $re_passwd ${new_conf_file} ${re_conf_path}
echo "Update the conf file ${re_conf_path} on ${re_ip}"

# End of script
