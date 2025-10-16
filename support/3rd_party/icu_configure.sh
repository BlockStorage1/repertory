#!/usr/bin/env bash

if [ "$(uname -m)" == "arm64" ] &&
  [ "${PROJECT_IS_ARM64}" == "0" ]; then
  HOST_CFG="--host=x86_64-apple-darwin"
  export CC="clang -arch x86_64"
  export CXX="clang++ -arch x86_64"
fi

CXXFLAGS="-std=gnu++17 -march=$1 -mtune=generic" ./configure \
  --disable-samples \
  --disable-tests \
  --enable-shared=$3 \
  --enable-static=yes \
  --prefix="$2" \
  ${HOST_CFG}
