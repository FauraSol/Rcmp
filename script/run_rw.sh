#!/bin/bash

port=$((14800+$1))

sudo gdb --args ../build/test/rw --client_ip=192.168.1.51 --client_port=$port --rack_id=$1 --cxl_devdax_path=/dev/shm/cxlsim$1 --cxl_memory_size=4294967296 --iteration=1000000 --payload_size=64 --start_addr=2097152 --alloc_page_cnt=100 --addr_range=209715200 --read_ratio=50