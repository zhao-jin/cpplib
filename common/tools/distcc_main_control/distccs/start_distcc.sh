#!/bin/bash
#
# Copyright (c) 2012, Tencent Inc.
#
# Author      : Michaelpeng <michaelpeng@tencent.com>
# Date        : Mar 20, 2012
#
# Description : To start distcc daemon


source './distccd_conf.sh'

echo `which distccd`
echo ""
echo `distccd --version | head -n1`
echo ""
echo `gcc --version | head -n1`

DISTCC_ARGS="--daemon --allow $SUBNET --listen $LOCAL_ADDR --port $PORT --log-file=$DISTCC_LOG --log-level=error"

echo "starting distccd as daemon"
command="distccd $DISTCC_ARGS"
echo "$command"
distccd $DISTCC_ARGS
echo "done..."
