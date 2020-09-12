#!/bin/sh
export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH
./core_server core_server_config.json config.json
./Idaemon run ./config.json &
