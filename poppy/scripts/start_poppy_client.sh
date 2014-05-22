#/bin/bash

if [ $# -ne 1 ]
then
   echo "start_poppy_client <latency|read|write|read_write>"
   exit 1
fi

nohup ./back_start_poppy_client.sh $1 > /dev/null 2>&1 &
