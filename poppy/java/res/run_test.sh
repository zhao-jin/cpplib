#!/bin/sh

if [ $# -lt 2 ]; then
    echo "Usage: " $0 " release|debug total_request_count"
    exit 1
fi

cp sync_client async_client poppy_test log4j.properties ../../../build64_$1/poppy/java

(
cd ../../../build64_$1/poppy/java

echo "Excute poppy sync test"
./sync_client "127.0.0.1:50000" $2
echo

echo "Excute poppy async test"
./async_client "127.0.0.1:50000" $2
echo

#echo "Excute poppy_test"
#./poppy_test
#echo
)
