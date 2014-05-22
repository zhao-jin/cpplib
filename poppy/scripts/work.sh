#!/bin/sh

ipfile=$1
expscript="./remote_exec.exp"
user=$2
passwd=$3

help()
{
	echo "Usage: $1 <ipfile> <user> <passwd> <cmd> <parameters>"
	echo "功能:在远程机器上执行一预定义类型的命令"
	echo "cmd 有以下几种，后面分别是相应的参数"
	echo "cmdline,后面接在远程机器执行的命令语句,该命令在远程机器运行一行命令，本地显示结果，如$1 cmdline \"free | tail -n 1 | awk '{print $3}'\""
	echo "cmdbat,后面接在远程机器需要执行的文件名字，该命令在远程机器运行一个文件,带显式路径则直接执行该文件，否则到某个默认目录执行"
	echo "copyto,后面接本机的文件名字和远程路径，该命令将本地文件复制到远程目录"
	echo "copyfrom,后面接远程机器的文件名字和本地路径，该命令将远程文件复制到本地"
	exit 1
}
#在远程机器上执行某命令()
cmdline()
{
	if [ $# -lt 1 ];then
		#cmd="ps auxwww | grep ucdbviewer"
		cmd="pwd"
	else
		cmd='$1'
	fi
	for i in `cat "$ipfile"`
	do
		tail=`echo $i | awk -F"." '{print $NF}'`
		#echo "passwd[$tail]"
		echo "ip:"$i
		eval "$expscript ssh $user $passwd  $i \"$cmd\" a.out"
		#sleep 1
	done
}

#复制本机器某文件到远程
copyto()
{
	if [ $# -lt 2 ];then
		echo "copy() need source file"
		exit 1
	fi
	#echo "copying"
	for i in `cat $ipfile`
	do
		tail=`echo $i | awk -F"." '{print $NF}'`
		eval "$expscript scp $user $passwd $i $1 $2 "	
		#sleep 1
	done
}
copyfrom()
{
	for i in	`cat $ipfile` 
	do
		#tail=`echo $i | awk -F"." '{print $NF}'`
		IPNAME=$i	
		name=`basename $1`
		eval "$expscript scpfrom $user $passwd $i $1 ./ "
		#mv ./$name ./${name}"."$tail
		mv ./$name $2/$IPNAME"."${name}
	done
}
cmdbat()
{
	if [ $# -lt 1 ];then
		echo "need processing file"
		exit 2
	fi
	cmdline " chmod +x $1;  $1 1>> .msg 2>>.err &\n"
}

proc()
{
	if [ $# -lt 1 ];then
		echo "need cmd"
		exit 3
	fi
	cmd=${2##\./}
	cmdline "cd $path1;./$cmd >/dev/null &"
	cmdline "cd $path2;./$cmd >/dev/null &"
}

case $4E in
	cmdlineE)
		if [ $# -lt 5 ];then
			cmdline	
		else
			cmdline "$5"
		fi
		;;
	copytoE)
		if [ $# -lt 5 ];then
			echo "need copy target"
			help $0
			exit 1
		fi
		if [ $# -lt 6 ];then
			#没有指定远程目录，默认复制到两个数据目录下
#			copyto $5 $path1
#			cmdline "cp $path1/$5 $path2"
			echo "need dest path"
			exit 3
		else
			copyto $5 $6
		fi	
		;;
	copyfromE)
		if [ $# -lt 5 ];then
			echo "need dest target"
			help $0
		fi
		if [ $# -lt 6 ];then
			copyfrom $5 "./"
		else
			copyfrom $5 $6
		fi
		;;
	cmdbatE)
		#echo "procfile"
		if [ $# -lt 5 ];then
			echo "need processing file"
			help $0
		fi
		cmdbat $5
		;;
	procE)
		if [ $# -lt 5 ];then
			help $0
		fi
		proc "$4"
		;;
	*)
		help $0
		;;
esac
exit 0
#cmd="cd /data/twse_spider/uc/ucdb/; chmod +x run.sh; ./run.sh"
##cmd="ps auxww | grep \"ucdbviewer\" | awk \'{print \$2}\' | xargs kill -9"
#for i in $ip:
##对每个ip,执行操作，该操作命令可以写在另外一个脚本里，也可以修改以上cmd变量实现
#do
#	curip=${i%%":"}
##	echo $curip
#	#为执行远程复合操作，将本地命令拷贝到远程机器 ，然后执行命令
#	 #./scp.sh webspider crawler $curip "$cmd" ${curip##'172.23.23.'}.out
#	 #获取远程机器数据,cmd变量只是充当占位符号
#	 ./getresult.exp webspider crawler $curip "$cmd" ${curip##'172.23.23.'}.out
##	./scp.sh $arg
#done
