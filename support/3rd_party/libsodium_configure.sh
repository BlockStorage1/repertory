#!/usr/bin/env bash

if [ "$1" == "mingw64" ]; then
  HOST_CFG=--host=x86_64-w64-mingw32
elif [ "$(uname -s)" == "Darwin" ] && [ "$(uname -m)" == "arm64" ] &&
  [ "${PROJECT_IS_ARM64}" == "0" ]; then
  HOST_CFG="--host=x86_64-apple-darwin"
  export CC="clang -arch x86_64"
  export CXX="clang++ -arch x86_64"
fi

CFLAGS="-O3 -fomit-frame-pointer -march=$2 -mtune=generic" ./configure \
  --disable-asm \
  --disable-ssp \
  --enable-shared=$4 \
  --enable-static=yes \
  --prefix="$3" \
  ${HOST_CFG}
