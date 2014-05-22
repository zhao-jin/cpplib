#!/bin/bash
#
# Copyright (c) 2012, Tencent Inc.
#
# Author      : Michaelpeng <michaelpeng@tencent.com>
# Date        : Mar 20, 2012
#
# Description : This script is used to check all the distccd running well


# Check the number of arguments $1 - mirror id
if [ $# -lt 1 ]
then
    prog=`basename $0`
    printf "usage:$prog mirror_id\n"
    exit 1
fi

mirr_id=$1
source const_ip_mirr_${mirr_id}.sh

total_machine_num=${#ip[@]}
echo "Checking distccd ${total_machine_num} on remote machines"

n=0
while [ $n -lt $total_machine_num ]
do
    ./check_remote.sh ${mirr_id} ${n}
    n=$((n+1))
    echo ""
done

# End of script

