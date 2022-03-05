#!/bin/bash

mkdir -p ../build/debug;cd ../build/debug && cmake ../.. -DREPERTORY_ENABLE_SKYNET=ON \
  -DREPERTORY_ENABLE_S3=ON \
  -DREPERTORY_ENABLE_SKYNET_PREMIUM_TESTS=ON \
  -DREPERTORY_ENABLE_S3_TESTING=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DREPERTORY_WRITE_SUPPORT=ON && make -j10 || exit 1

cd ..

ln -sf debug/repertory .
ln -sf debug/unittests . 
ln -sf debug/compile_commands.json . 
