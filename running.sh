#!/bin/bash

cd src
make clean
make
cd ../tests
make clean
make
icc BlockCCM-MT-App.cc -o BlockCCM-MT-App -std=c++11 -I../src -L../lib -lstdc++ -lEDM -lpthread -O3 -DASYNC_THREAD_WRITE -g -DDEBUG
cd ..
amplxe-cl -collect hotspots -knob sampling-mode=hw -knob sampling-interval=1 -data-limit=0 /home/boom/cppEDM/tests/FishTestApp