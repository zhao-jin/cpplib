#!/bin/bash
#
# Copyright (c) 2012, Tencent Inc.
#
# Author      : Michaelpeng <michaelpeng@tencent.com>
# Date        : Mar 20, 2012
#
# Description : This script is used to host the common functions defined


##########################################################
############### DEFINE FUNCTIONS HERE ####################
##1 - SIGHUP 2 - SIGINT 3 - SIGQUIT 15 - SIGTERM
##9 - SIGKILL cannot be trapped
##########################################################

# Do clearing work before exiting
function trap_exit
{
    echo 'Exiting on a trapped signal'
}

trap 'trap_exit;exit 0' 1 2 3 15

### $1-ip $2-port $3-user $4-passwd $5-process name
function get_rproc_num
{
    if [[ $# -ne 5 ]]
    then
        echo "usage: get_rproc_num ip port user passwd process_name"
        return 1
    fi
    rcmd="ps aux | grep $5 | grep -v grep | wc -l"
    ret=`./run_remote.exp $1 $2 $3 $4 "$rcmd"`
    echo "${ret}" >get_rproc.tmp
    rproc_num=`tail -n1 get_rproc.tmp`
    rproc_num=$(( $rproc_num + 0))

    echo "${rproc_num} $5 is(are) running on remote $1"
    if [ -f ./get_rproc.tmp ]
    then
        rm ./get_rproc.tmp
    fi
    return ${rproc_num}
}

### $1-ip $2-port $3-user $4-passwd $5-process name
function check_rproc_exist
{
    get_rproc_num $1 $2 $3 $4 $5
    [[ $? -ne 0 ]] && echo "The proc $5 exists" && return 0
    return 1
}

### $1-file_name $2-key $3-new_value
function set_conf_value
{
    if [[ $# -ne 3 ]]
    then
        echo "usage:set_conf_file filename key new_value"
        return 1
    fi

    file="$1"
    key="$2"
    new_value="$3"

    file_existed=1

    if [[ ! -f $1 ]]
    then
        echo "Not existed conf file, will setup one"
        file_existed=0
    fi

    if [ $file_existed -eq 1 ]
    then
        ret=`grep "^${key}=" $file`
        if [[ -z "$ret" && "$ret" == '' ]]
        then
            echo "Not existed key ${key} in $1, will do nothing"
            return 1
        fi
    fi

    if [ $file_existed -eq 1 ]
    then
        sed -i "s%${key}=.*%${key}=${new_value}%" $file
    else
        echo ${key}=${new_value} >>$file
    fi
    return 0
}

# warn users to proceed
function warning
{
    echo "sure to continue?(yes/no)"
    read answer
    [[ ! -z "$answer" && "$answer" != "yes" ]] && echo "exit" && exit 1
    [[ ! -z "$answer" && "$answer" == "yes" ]] && echo "continue"
    return 0
}

# End of script

