pid=`cat server.pid`

kill -9 $pid

killall echoserver
