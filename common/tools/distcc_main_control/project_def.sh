#!/bin/bash
#
# Copyright (c) 2012, Tencent Inc.
#
# Author      : Michaelpeng <michaelpeng@tencent.com>
# Date        : Mar 20, 2012
#
# Description : This script is used to host the project functions defined


proj_name="distccs"
conf_file_name="distccd_conf.sh"

# $1-source folder $2- tar ball name
function pack_source
{
    if [ $# -ne 2 ]
    then
        echo "usage:pack_source source_base_dir tar_ball_name"
        return 1
    fi

    source_base=$1
    tar_ball_name=$2

    tar -czf ${tar_ball_name} ${source_base}

    [[ ! -f ${tar_ball_name} ]] && echo "compressed tarball ${tar_ball_name} failed" && return 1

    echo "compressed tar ball ${tar_ball_name} succeeded"
    return 0
}

function remove_proj_files
{
    if [ $# != 5 ]
    then
        printf "usage:remove_proj_files ip ssh_port user passwd dest_suite_name"
        return 1
    fi

    re_path=$5
    re_proj_path="${re_path}/${proj_name}"

    # check that we don't remove the vip files
    #[[ -z "${re_dest_dir}" && "${re_dest_dir}" == '' ]] && echo "WARN:going to remove the '/' or sub dirs" && warning
    [[ "$re_path" == "/" || "$re_path" == "/bin" || "$re_path" == "/usr" || "$re_path" == "/lib" || "$re_path" == "/etc" || "$re_path" == "/lib64" || "$re_path" == "/dev" || "$re_path" == "/data" || "$re_path" == "/opt" || "$re_path" == "/var" ]] && echo "WARN:going to remove the '/' or sub dirs" && warning

    ./run_remote.exp $1 $2 $3 $4 "[[ -d ${re_proj_path}  ]] && rm -rf ${re_proj_path}"

    return 0
}

# copy remain folders like data folder which we don't want to compress to remote
# $1-remote machine, left blank here
function rcp_remain
{
    return 0
}

# End of script

