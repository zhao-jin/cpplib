#!/bin/bash
#
# Copyright (c) 2012, Tencent Inc.
#
# Author      : Michaelpeng <michaelpeng@tencent.com>
# Date        : Mar 20, 2012
#
# PURPOSE : To stop distcc daemon

source './distccd_conf.sh'

echo 'stopping distccd'
killall distccd
echo 'done'
