#!/bin/bash
#
# Copyright (c) 2012, Tencent Inc.
#
# Author      : Michaelpeng <michaelpeng@tencent.com>
# Date        : Mar 20, 2012
#
# Description : This script is used to install the package to all
#               remote machines which are defined in const definition file.


source common_def.sh

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
echo "Deploying packages to ${total_machine_num} remote machines"

echo "WARN:Will remove the old compressed tarballs here and remove the remote packages"
warning

has_tar_ball=`ls -l . | grep '.tar.gz' | wc -l`
[[ ${has_tar_ball} -ne 0 ]] && rm ./*.tar.gz

n=0
while [ $n -lt $total_machine_num ]
do
    ./install_remote.sh ${mirr_id} ${n}
    n=$((n+1))
    echo ""
done

# End of script

