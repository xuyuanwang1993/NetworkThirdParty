export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH
killall -9 Idaemon
./Idaemon exit ./config.json &
