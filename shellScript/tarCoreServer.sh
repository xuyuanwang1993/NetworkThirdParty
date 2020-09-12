#!/bin/sh
# 拷贝核心服务器所需的资源
#./*.sh <out_path>
#
if [ $# -le 0 ]
then
	printf "please specify the out_path!\n"
	exit 0
fi
sources_name=core_server
out_path=$1
echo "out_path=$out_path"
#check_port_owner.sh
sources="check_port_owner.sh"
#run.sh
sources="${sources} run.sh"
#set_env.readme
sources="${sources} set_env.readme"
#exit.sh
sources="${sources} exit.sh"
#core_server
sources="${sources} ${out_path}/bin/${sources_name}/*"
#daemon
sources="${sources} ${out_path}/bin/Idaemon/*"
echo $sources
mkdir -p $sources_name
cp -ruf  $sources $sources_name
tar -cvf ${sources_name}.tar $sources_name
rm $sources_name -rf
