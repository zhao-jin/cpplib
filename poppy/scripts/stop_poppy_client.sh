pid=`cat client.pid`

kill -9 $pid

killall echoclient
