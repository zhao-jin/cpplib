#!/bin/bash
#
# Copyright (c) 2012, Tencent Inc.
#
# Author      : Michaelpeng <michaelpeng@tencent.com>
# Date        : Mar 20, 2012
#
# Description : This script is used to check distccd daemon remotely


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

# create dir at remote machine and copy tarball to remote
proj_base="${re_dest_dir}/${proj_name}/${proj_name}"
script_base="${proj_base}"

./run_remote.exp $re_ip $re_ssh_port $re_user $re_passwd "cd ${script_base} && sh ./check_distcc.sh"

# End of script

