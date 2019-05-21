#!/bin/bash

cd src
make clean
make
cd ../tests
make clean
make
cd ..
amplxe-cl -collect hotspots -knob sampling-mode=hw -knob sampling-interval=1 -result-dir profiler /home/boom/cppEDM/tests/FishTestApp