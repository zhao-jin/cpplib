#!/bin/bash
#
# Copyright (c) 2012, Tencent Inc.
#
# Author      : Michaelpeng <michaelpeng@tencent.com>
# Date        : Mar 20, 2012
#
# Description : This script is used to install the package to all
#               remote machines which are defined in const definition file.


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

# Get the tar ball
base="${proj_name}"
tarball="${proj_name}.tar.gz"

[[ ! -f $tarball ]] && pack_source $base $tarball && echo "get new tar ball"

# create dir at remote machine and copy tarball to remote
re_path="${re_dest_dir}/${proj_name}"

./run_remote.exp $re_ip $re_ssh_port $re_user $re_passwd "mkdir -p ${re_path}"
./rcp.exp $re_ip $re_ssh_port $re_user $re_passwd ${tarball} ${re_path}
./run_remote.exp $re_ip $re_ssh_port $re_user $re_passwd "cd ${re_path} && tar -xzf ${tarball} && rm ./${tarball}"

./proj_update_remote_conf.sh ${mirr_id} ${remote_id}

# End of script
