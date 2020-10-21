#!/bin/sh
# 拷贝核心服务器所需的资源
#./*.sh <out_path>
#
if [ $# -le 0 ]
then
	printf "please specify the out_path!\n"
	exit 0
fi
sources_name=proxy_server
web_base_dir=web_dir
web_work_dir=${PWD}/$web_base_dir
web_src_dir=../web
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
#coyp web
make -C ${web_src_dir} -f ${web_src_dir}/Makefile.cross OUT_PATH=${out_path} PREFIX=${web_work_dir} install
echo $sources
mkdir -p $sources_name
cp -ruf  $sources $sources_name
sed -i "s/core_server/${sources_name}/g" $sources_name/run.sh
#tar 
tar -cvf ${sources_name}.tar $sources_name $web_base_dir installProxyserver.sh
rm $sources_name $web_work_dir -rf
