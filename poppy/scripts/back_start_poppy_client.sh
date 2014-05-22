#/bin/bash
# author: hsiaokangliu

if [ $# -ne 1 ]
then
   echo "start_poppy_client <latency|read|write|read_write>"
   exit 1
fi

ulimit -c unlimited

address_file="arguments.cfg"
line_number=0
ip_address=""
while read line; do
    if [ $line_number -eq 0 ]; then
        ip_address=$line
    else
        ip_address="$ip_address,$line"
    fi
    line_number=`expr $line_number + 1`
done < $address_file

index=0
start_time=`date +%s`
echo "client started at the ${index}th time." > log_$index.txt
echo "client started at time: `date`" >> log_$index.txt
nohup ./echoclient --server_address=$ip_address --$1=true >> log_$index.txt 2>&1 &

# Failed to start the client.
if [ $? -ne 0 ]; then
    echo "failed to start the client. exit." >> log_$index.txt
    exit 1
fi

pid=$!
echo $$ > client.pid

while true
do
    if ! kill -0 $pid 2>/dev/null ; then        # no such a process, have stopped.
        echo "client stoped at time: `date`" >> log_$index.txt
        index=`expr $index + 1`
        echo "client started at the ${index}th time." > log_$index.txt
        echo "client started time: `date`" >> log_$index.txt
        nohup ./echoclient --server_address=$ip_address --$1=true >> log_$index.txt 2>&1 &
        if [ $? -ne 0 ]; then
            echo "failed to start the client. exit." >> log_$index.txt
            exit 1
        fi
        pid=$!
    else
        top -p $pid -b -n 1 >> monitor_$index.txt
    fi

    sleep 1;
done
