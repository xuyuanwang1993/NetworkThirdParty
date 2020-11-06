#!/bin/sh
# 
#./*.sh <port> <program_name><tcp/udp>
#
pro_port=$1
pro_name=$2
socket_mode=$3
########################check input###############################
if  [ ${socket_mode} != tcp ] && [ ${socket_mode} != udp ];then
	printf "exit false check mode %s \n" $socket_mode
	exit 0
fi

if  test -z ${pro_port}  ||  test -z ${pro_name}  ||  test -z ${socket_mode};then
	printf "exit false input for PRO_PORT:%s PRO_NAME %s \n " $pro_port $pro_name
	exit 0
else	
if test $((pro_port)) -gt 0  &&  test $((pro_port)) -le 65535; then
	printf ""
#	printf "input PRO_PORT:%s PRO_NAME:%s mode:%s\n " $pro_port $pro_name $socket_mode
else
	printf "exit false input port %s range(1-65535)\n" $pro_port
	exit 0
fi
fi
####################################################################

###########################mode  tcp########################################
if [ ${socket_mode} = tcp ];then
match_string=`netstat -tnlp | grep ":$pro_port "`
temp=`echo $match_string | awk '{print $7}'`
pid=`echo $temp | awk -F "/" '{print $1}'`
if [ -z $pid ];then
	echo "can't find $pro_name $pro_port $socket_mode!"
	exit 0
fi
#end fo get pid
#program_name=`echo $temp | sed "s/\([0-9]*\)\/\(.*\)/\2/g"`
program_name=`ps -a | grep $pid |awk '{print $4}' | grep -v grep`
if [ -z $program_name ];then
	echo "can't find $pro_name $pro_port $socket_mode!"
	exit 0
fi
#endi for match program_name
program_name=`echo $program_name | awk -F "/" '{print $NF}'`
if [ ${pro_name} != ${program_name} ];then
	echo "$pro_port $pro_name $socket_mode not matched and kill it!"
	kill -9 $pid
else
	printf ""
#	echo "$pro_name($pid) $pro_port $socket_mode matched!"
fi
###############################mode udp###############################################
else
match_string=`netstat -unlp | grep ":$pro_port "`
temp=`echo $match_string | awk '{print $6}'`
pid=`echo $temp | awk -F "/" '{print $1}'`
if [ -z $pid ];then
	echo "can't find $pro_name $pro_port $socket_mode!"
	exit 0
fi
#program_name=`echo $temp | sed "s/\([0-9]*\)\/\(.*\)/\2/g"`
program_name=`ps -a | grep $pid |awk '{print $4}' | grep -v grep`
if [ -z $program_name ];then
	echo "can't find $pro_name $pro_port $socket_mode!"
	exit 0
fi
program_name=`echo $program_name | awk -F "/" '{print $NF}'`
if [ ${pro_name} != ${program_name} ];then
	echo "$pro_name $pro_port $socket_mode not matched $program_name and kill it!"
	kill -9 $pid
else
	printf ""
#	echo "$pro_name($pid) $pro_port $socket_mode matched!"
fi
fi
