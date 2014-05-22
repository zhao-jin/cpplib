#!/bin/bash
# 版本不低于5.3.8
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

if [ $# -lt 1 ]; then
    echo "Usage: " $0 " release|debug"
    exit 1
fi

BLADE_ROOT_DIR=../../../../../

mkdir -p $BLADE_ROOT_DIR/build64_$1/common/zookeeper/client/tests/php 2>/dev/null

cp zk_client_test.php $BLADE_ROOT_DIR/build64_$1/common/zookeeper/client/tests/php
cp php.ini $BLADE_ROOT_DIR/build64_$1/common/zookeeper/client/tests/php
cd $BLADE_ROOT_DIR/build64_$1/common/zookeeper/client
cp zk_client.so  tests/php/
cp zk_client.php tests/php/

cd tests/php/
php -c php.ini zk_client_test.php --ini

