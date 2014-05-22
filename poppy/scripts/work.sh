#!/bin/sh

ipfile=$1
expscript="./remote_exec.exp"
user=$2
passwd=$3

help()
{
	echo "Usage: $1 <ipfile> <user> <passwd> <cmd> <parameters>"
	echo "����:��Զ�̻�����ִ��һԤ�������͵�����"
	echo "cmd �����¼��֣�����ֱ�����Ӧ�Ĳ���"
	echo "cmdline,�������Զ�̻���ִ�е��������,��������Զ�̻�������һ�����������ʾ�������$1 cmdline \"free | tail -n 1 | awk '{print $3}'\""
	echo "cmdbat,�������Զ�̻�����Ҫִ�е��ļ����֣���������Զ�̻�������һ���ļ�,����ʽ·����ֱ��ִ�и��ļ�������ĳ��Ĭ��Ŀ¼ִ��"
	echo "copyto,����ӱ������ļ����ֺ�Զ��·��������������ļ����Ƶ�Զ��Ŀ¼"
	echo "copyfrom,�����Զ�̻������ļ����ֺͱ���·���������Զ���ļ����Ƶ�����"
	exit 1
}
#��Զ�̻�����ִ��ĳ����()
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

#���Ʊ�����ĳ�ļ���Զ��
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
			#û��ָ��Զ��Ŀ¼��Ĭ�ϸ��Ƶ���������Ŀ¼��
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
##��ÿ��ip,ִ�в������ò����������д������һ���ű��Ҳ�����޸�����cmd����ʵ��
#do
#	curip=${i%%":"}
##	echo $curip
#	#Ϊִ��Զ�̸��ϲ������������������Զ�̻��� ��Ȼ��ִ������
#	 #./scp.sh webspider crawler $curip "$cmd" ${curip##'172.23.23.'}.out
#	 #��ȡԶ�̻�������,cmd����ֻ�ǳ䵱ռλ����
#	 ./getresult.exp webspider crawler $curip "$cmd" ${curip##'172.23.23.'}.out
##	./scp.sh $arg
#done
