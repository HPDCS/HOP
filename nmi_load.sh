#!/bin/bash

echo "Building with NMI support" 

cd ./hop
make type=nmi
cd ../main
make
cd ../daemon
make
cd ../tests
make

cd ../

#sudo insmod ./schedule-hook/schedule-hook.ko
#lsmod | grep schedule_hook
sudo insmod ./hop/hop.ko
lsmod | grep hop

#cat /sys/module/hop/parameters/monitor_hook > /sys/module/schedule_hook/parameters/the_hook

echo "Done"
