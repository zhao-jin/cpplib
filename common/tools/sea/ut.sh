#!/bin/sh

if [ $# -lt 1 ]; then
    echo "Usage:$0 path"
    exit
fi
path=$1

allfile=`find $1 -name *_test.py` 

for i in $allfile;do
    pushd `pwd`
    #echo $i `dirname $i` `basename $i`
    echo $i && cd `dirname $i` && python `basename $i` 
    popd
done
