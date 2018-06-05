#!/bin/sh

# this should replaced by make
# cd ./schedule-hook
# make clean
cd ./hop
make clean
cd ../main
make clean
cd ../daemon
make clean

cd ../

sudo rmmod hop
echo "hop off"

echo "Done"
