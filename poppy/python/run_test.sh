#!/bin/sh

if [ $# -lt 2 ]; then
    echo "Usage: " $0 " release|debug total_request_count"
    exit 1
fi

mkdir -p ../../build64_$1/poppy/python/poppy 2>/dev/null

cp ../../build64_$1/poppy/_poppy_client.so ../../build64_$1/poppy/python/poppy
cp ../../build64_$1/poppy/poppy_client.py ../../build64_$1/poppy/python/poppy
cp poppy/client.py ../../build64_$1/poppy/python/poppy
cp poppy/__init__.py ../../build64_$1/poppy/python/poppy

cp poppy_simple.py simple_test.py test_utils.py sync_test.py async_test.py ../../build64_$1/poppy/python

(
cd ../..

thirdparty/protobuf/bin/protoc -I. -Ithirdparty --python_out=build64_$1 poppy/rpc_error_code_info.proto
cp build64_$1/poppy/rpc_error_code_info_pb2.py build64_$1/poppy/python/poppy

thirdparty/protobuf/bin/protoc -I. -Ithirdparty --python_out=build64_$1 poppy/rpc_option.proto
cp poppy/rpc_option.proto build64_$1/poppy/python/poppy
cp build64_$1/poppy/rpc_option_pb2.py build64_$1/poppy/python/poppy

thirdparty/protobuf/bin/protoc -I. -Ithirdparty --python_out=build64_$1 poppy/rpc_message.proto
cp poppy/rpc_message.proto build64_$1/poppy/python/poppy

thirdparty/protobuf/bin/protoc -I. -Ithirdparty --python_out=build64_$1 poppy/rpc_meta_info.proto
cp build64_$1/poppy/rpc_meta_info_pb2.py build64_$1/poppy/python

thirdparty/protobuf/bin/protoc -I. -Ithirdparty --python_out=build64_$1 poppy/python/echo_service.proto

cd - >/dev/null

cd ../../build64_$1/poppy/python

echo "Excute simple poppy test"
python simple_test.py $2
echo

echo "Excute poppy sync test"
python sync_test.py $2
echo

echo "Excute poppy async test"
python async_test.py $2 100
echo
)
