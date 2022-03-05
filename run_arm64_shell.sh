#!/bin/bash

NAME=$1
if [ -z "$NAME" ]; then
  echo "Name not set"
elif [ "$UID" != 0 ]; then
  echo "Must be run as root"
  exit 1
else
  DISTROPART=$NAME
  DISTRONAME=arm64_${DISTROPART}
  ARM64_DIR="./${DISTRONAME}"

  mount binfmt_misc -t binfmt_misc /proc/sys/fs/binfmt_misc
  echo -1 > /proc/sys/fs/binfmt_misc/qemu-aarch64 || true
  echo ':qemu-aarch64:M::\x7fELF\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\xb7\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/usr/bin/qemu-aarch64-static:OCF' > /proc/sys/fs/binfmt_misc/register

  if mount -o bind "$(pwd)" "${ARM64_DIR}/mnt"; then
    systemd-nspawn -D "${ARM64_DIR}"
    umount "${ARM64_DIR}/mnt"
  else
    echo "Mount failed: ${DISTRONAME}"
  fi
fi
