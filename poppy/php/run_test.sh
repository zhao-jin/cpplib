#!/bin/bash

# 要求 PHP cli 版本不低于5.3.8
php=0
php_version_info=`php --version 2>/dev/null`
if [ $? -eq 0 ]; then
    version=`echo $php_version_info | head -n 1 | awk '{print $2}'`
    if [[ "$version" > "5.3.07" ]]; then
        php=1
    fi
fi

if [ $php -ne 1 ]; then
    echo "Must install PHP 5.3.8 or above version"
    exit 1
fi

if [ $# -lt 2 ]; then
    echo "Usage: " $0 " release|debug total_request_count"
    exit 1
fi

BLADE_ROOT_DIR=../..

mkdir -p $BLADE_ROOT_DIR/build64_$1/poppy/php/poppy 2>/dev/null

(
cd $BLADE_ROOT_DIR

(
cd thirdparty/Protobuf-PHP/library && find DrSlump ! -path "*.svn*" -type f | xargs tar cf - | tar xf - -C ../../../build64_$1/poppy/php
)

mkdir -p build64_$1/poppy/php/thirdparty/google/protobuf/compiler
cp thirdparty/google/protobuf/descriptor.proto build64_$1/poppy/php/thirdparty/google/protobuf
cp thirdparty/google/protobuf/compiler/plugin.proto build64_$1/poppy/php/thirdparty/google/protobuf/compiler

mkdir -p build64_$1/poppy/php/thirdparty/protobuf/bin
cp thirdparty/protobuf/bin/protoc thirdparty/protobuf/bin/protoc.exe build64_$1/poppy/php/thirdparty/protobuf/bin

thirdparty/protobuf/bin/protoc --plugin=protoc-gen-php=thirdparty/Protobuf-PHP/protoc-gen-php.php -I. -Ithirdparty -Ithirdparty/Protobuf-PHP/library --php_out=build64_$1/poppy/php/poppy poppy/rpc_option.proto
cp poppy/rpc_option.proto build64_$1/poppy/php/poppy

cp poppy/rpc_message.proto build64_$1/poppy/php/poppy

thirdparty/protobuf/bin/protoc --plugin=protoc-gen-php=thirdparty/Protobuf-PHP/protoc-gen-php.php -I. -Ithirdparty -Ithirdparty/Protobuf-PHP/library --php_out=build64_$1/poppy/php/poppy poppy/rpc_error_code_info.proto

cp build64_$1/poppy/poppy_client.so build64_$1/poppy/php/poppy
cp build64_$1/poppy/poppy_client.php build64_$1/poppy/php/poppy
cp poppy/php/poppy/client.php build64_$1/poppy/php/poppy


thirdparty/protobuf/bin/protoc --plugin=protoc-gen-php=thirdparty/Protobuf-PHP/protoc-gen-php.php -I. -Ithirdparty -Ithirdparty/Protobuf-PHP/library --php_out=build64_$1/poppy/php poppy/examples/echo_sync/echo_service.proto
rm -rf build64_$1/poppy/php/descriptor.pb.php build64_$1/poppy/php/rpc_option.pb.php

cp poppy/php/php.ini poppy/php/sync_test.php build64_$1/poppy/php
)

(
cd $BLADE_ROOT_DIR/build64_$1/poppy/php

echo "Excute poppy sync test"
php -c php.ini sync_test.php $2
echo
)

#SWIG_LIB=thirdparty/swig/share/swig thirdparty/swig/bin/swig -php -c++ -verbose -o build64_release/poppy/poppy_client_phpwrap.cxx poppy/poppy_client.i

#thirdparty/protobuf/bin/protoc --plugin=protoc-gen-php=thirdparty/Protobuf-PHP/protoc-gen-php.php -I. -Ithirdparty -Ithirdparty/Protobuf-PHP/library --cpp_out=build64_release --python_out=build64_release --php_out=build64_release/poppy/php/poppy poppy/rpc_message.proto
