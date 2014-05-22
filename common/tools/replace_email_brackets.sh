#!/bin/sh
# author: simonwang
# replace parentheses of email address to angle brackets
# for example: <xxx@yy.com> to <xxx@yy.com> style

if [ -z "$1" ]
then
    echo "No root dir specified, please specify a root dir to start replace" >/dev/fd/2
    exit 1
else
    ROOT_DIR=$1
fi

find $ROOT_DIR ! -path "*.svn*" -type f | xargs sed -i -r  's/\((\w+([-+.]\w+)*@\w+([-.]\w+)*\.\w+([-.]\w+)*([,;]\s*\w+([-+.]\w+)*@\w+([-.]\w+)*\.\w+([-.]\w+)*)*)\)/<\1>/g'
