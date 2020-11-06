#!/bin/sh
#copy file to /usr/local/sbin
chmod -R 755 web_dir/ proxy_server
cp -ruf web_dir/ proxy_server /usr/local/sbin
script_name=start_proxyserver.sh
rm $script_name -rf
#generate auto run script
echo '#/!/bin/sh' >>$script_name
echo '#start web' >>$script_name
echo 'killall -9 boa' >>$script_name
echo '/usr/local/sbin/web_dir/bin/boa -c /usr/local/sbin/web_dir/config' >>$script_name
echo '#start proxy_server' >>$script_name
echo 'rm /tmp/daemon_instance -rf' >>$script_name
echo 'cd /usr/local/sbin/proxy_server/;./exit.sh' >>$script_name
echo 'cd /usr/local/sbin/proxy_server/;./run.sh' >>$script_name
chmod 755 $script_name
mv $script_name /usr/local/sbin
echo "if you want to add an reboot-auto run task,try run command as root:\"crontab -e\" and add the following line"
echo "@reboot ( sleep 60 ; sh /usr/local/sbin/${script_name})"
exit 0
