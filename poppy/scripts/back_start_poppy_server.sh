#/bin/bash
#author: hsiaokangliu

if [ $# -ne 1 ]
then
   echo "Usage: start_poppy_server <port>\n"
   exit 1
fi

rm -f server.pid
ulimit -c unlimited
ip_address=`/sbin/ifconfig -a | grep inet | grep -v 127.0.0.1 | awk '{print $2}'| tr -d "addr:"`
port=$1

index=0
start_time=`date +%s`
echo "server started at the ${index}th time." > log_$index.txt
echo "server started at time: `date`" >> log_$index.txt
nohup ./echoserver --server_address=${ip_address}:${port} >> log_$index.txt 2>&1 &

# Failed to start the server. Maybe port is already in use.
if [ $? -ne 0 ]; then
    echo "failed to start the server. exit." >> log_$index.txt
    exit 1
fi

pid=$!
echo $$ > server.pid

while true
do
    end_time=`date +%s`
    past_time=`expr $end_time - $start_time`

    if ! kill -0 $pid 2>/dev/null ; then        # no such a process, have stopped.
        echo "server stoped at time: `date`" >> log_$index.txt
        start_time=`date +%s`
        index=`expr $index + 1`
        echo "server started at the ${index}th time." > log_$index.txt
        echo "server started time: `date`" >> log_$index.txt
        nohup ./echoserver --server_address=${ip_address}:${port} >> log_$index.txt 2>&1 &
        if [ $? -ne 0 ]; then
            echo "failed to start the server. exit." >> log_$index.txt
            exit 1
        fi
        pid=$!
    else
        top -p $pid -b -n 1 >> monitor_$index.txt
    fi

    sleep 1;
done

