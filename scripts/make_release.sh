#!/bin/bash

mkdir -p ../build/release;cd ../build/release && cmake ../.. -DREPERTORY_ENABLE_SKYNET=ON \
  -DREPERTORY_ENABLE_S3=ON \
  -DREPERTORY_ENABLE_SKYNET_PREMIUM_TESTS=ON \
  -DREPERTORY_ENABLE_S3_TESTING=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DREPERTORY_WRITE_SUPPORT=ON && make -j10 || exit 1

cd ..

ln -sf release/repertory .
ln -sf release/unittests . 
ln -sf release/compile_commands.json . 
