#!/bin/sh
export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH
rm /tmp/daemon_instance -rf
rm config.json -rf
./core_server core_server_config.json config.json
./Idaemon run ./config.json &
