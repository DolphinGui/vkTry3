#!/bin/bash

cmake -DCMAKE_BUILD_TYPE=Debug -S src/ -B build/
cd build
make
cd ..
