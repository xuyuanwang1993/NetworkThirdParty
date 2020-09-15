#!/bin/sh
export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH
rm config.json
./core_server core_server_config.json config.json
./Idaemon run ./config.json &
