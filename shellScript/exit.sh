export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH
rm /tmp/daemon_instance -rf
killall -9 Idaemon
./Idaemon exit ./config.json
