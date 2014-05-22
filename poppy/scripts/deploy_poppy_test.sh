#!/bin/sh
#author: hsiaokangliu

expscript="./remote_exec.exp"
user=xfs_admin
passwd=xfspublic

total_round=800
echoserver="./echoserver"
echoclient="./echoclient"
server_ip_file="server_list.txt"
client_ip_file="client_list.txt"
client_argument="arguments.cfg"
server_dir="/data/poppy_test/server"

port=50000
test_type="run_latency_test"

deploy_server()
{
    # clear data
    ./work.sh $server_ip_file $user $passwd cmdline "rm -rf $server_dir"
    # create a test directory
    ./work.sh $server_ip_file $user $passwd cmdline "mkdir -p $server_dir"
    # copy server binary to the directory
    ./work.sh $server_ip_file $user $passwd copyto $echoserver $server_dir
    # copy start/stop server script to the directory
    ./work.sh $server_ip_file $user $passwd copyto ./start_poppy_server.sh $server_dir
    ./work.sh $server_ip_file $user $passwd copyto ./back_start_poppy_server.sh $server_dir
    ./work.sh $server_ip_file $user $passwd copyto ./stop_poppy_server.sh $server_dir
}

start_server()
{
    ./work.sh $server_ip_file $user $passwd cmdline "cd $server_dir; ./start_poppy_server.sh $port"
}

stop_server()
{
    ./work.sh $server_ip_file $user $passwd cmdline "cd $server_dir; ./stop_poppy_server.sh"
}

deploy_client()
{
    round=0
    client_dir="/data/poppy_test/client_$round"
    # clear data
    ./work.sh $client_ip_file $user $passwd cmdline "rm -rf /data/poppy_test"
    # create a test directory
    ./work.sh $client_ip_file $user $passwd cmdline "mkdir -p $client_dir"
    # copy client binary to the directory
    ./work.sh $client_ip_file $user $passwd copyto $echoclient $client_dir
    # copy server list to the directory
    ./work.sh $client_ip_file $user $passwd copyto $client_argument $client_dir
    # copy start/stop client script to the directory
    ./work.sh $client_ip_file $user $passwd copyto ./start_poppy_client.sh $client_dir
    ./work.sh $client_ip_file $user $passwd copyto ./back_start_poppy_client.sh $client_dir
    ./work.sh $client_ip_file $user $passwd copyto ./stop_poppy_client.sh $client_dir
}

deploy_multi_client()
{
    round=1
    command=""
    while [ $round -lt $total_round ]; do
        client_dir="/data/poppy_test/client_$round"
        dir_0="/data/poppy_test/client_0"
        command="cp $dir_0 $client_dir -a; $command"
        round=`expr $round + 1`
    done
    ./work.sh $client_ip_file $user $passwd cmdline "$command"
}

start_client()
{
    round=0
    command=""
    while [ $round -lt $total_round ]; do
       client_dir="/data/poppy_test/client_$round"
       command="$command cd $client_dir; ./start_poppy_client.sh $test_type;"
       round=`expr $round + 1`
    done
    ./work.sh $client_ip_file $user $passwd cmdline "$command"
}

stop_client()
{
    round=0
    command=""
    while [ $round -lt $total_round ]; do
        client_dir="/data/poppy_test/client_$round"
        command="$command cd $client_dir; ./stop_poppy_client.sh;"
        round=`expr $round + 1`
    done
    ./work.sh $client_ip_file $user $passwd cmdline "$command"
}

remove_binary()
{
    dir="/data/poppy_test/"
    ./work.sh $server_ip_file $user $passwd cmdline "rm -rf $dir"
    ./work.sh $client_ip_file $user $passwd cmdline "rm -rf $dir"
}

clear_logs()
{
    rm_cmd="rm -f *.log *.INFO *.WARNING *.ERROR *.txt core*"
    ./work.sh $server_ip_file $user $passwd cmdline "cd $server_dir; $rm_cmd"
    round=0
    command=""
    while [ $round -lt $total_round ]; do
        client_dir="/data/poppy_test/client_$round"
        command="$command cd $client_dir; $rm_cmd;"
        round=`expr $round + 1`
    done
    ./work.sh $client_ip_file $user $passwd cmdline "$command"
}

if [ $# -lt 1 ]
then
    echo "deploy_poppy_test start [test_type] | stop."
    exit 1
fi

if [ $1 == "deploy" ]; then
    deploy_server
    deploy_client
    deploy_multi_client
elif [ $1 == "start" ]; then
    start_server
    if [ $# -eq 2 ]; then
        test_type="run_${2}_test"
    fi
    start_client
elif [ $1 == "stop" ]; then
    stop_client
    stop_server
elif [ $1 == "clear" ]; then
    clear_logs
elif [ $1 == "remove" ]; then
    remove_binary
fi

