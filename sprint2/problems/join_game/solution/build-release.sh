#!/bin/bash
rm -rf ./build-release
mkdir build-release
cd build-release

conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
