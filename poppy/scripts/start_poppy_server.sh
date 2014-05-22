#/bin/bash

if [ $# -ne 1 ]
then
   echo "Usage: start_poppy_server <port>\n"
   exit 1
fi

nohup ./back_start_poppy_server.sh $1 > /dev/null 2>&1 &

