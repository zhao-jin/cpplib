#!/bin/bash
#
# Copyright (c) 2012, Tencent Inc.
#
# Author      : Michaelpeng <michaelpeng@tencent.com>
# Date        : Mar 20, 2012
#
# Description : This script is used to check the distccd locally


source './distccd_conf.sh'

running_daemons=`ps aux | grep 'distccd' | grep -v grep | wc -l`
if [ $running_daemons -eq 0 ]
then
    echo 'WARNING, distccd not running'
else
    echo 'ok'
fi
