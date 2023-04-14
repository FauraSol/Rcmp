#!/bin/bash

port=$((14800+$1))

sudo ../build/multi_rack_client --client_ip=192.168.1.51 --client_port=$port --rack_id=$1 --cxl_devdax_path=/dev/shm/cxlsim$1 --cxl_memory_size=4294967296