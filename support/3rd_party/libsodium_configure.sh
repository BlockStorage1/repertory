#!/bin/bash

if [ "$1" == "mingw64" ]; then
  HOST_CFG=--host=x86_64-w64-mingw32
fi

CFLAGS="-O3 -fomit-frame-pointer -march=$2 -mtune=generic" ./configure \
  --disable-asm \
  --disable-ssp \
  --enable-shared=$4 \
  --enable-static=yes \
  --prefix="$3" \
  ${HOST_CFG}
