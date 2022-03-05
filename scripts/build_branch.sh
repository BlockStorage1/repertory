#!/bin/bash

echo -1 > /proc/sys/fs/binfmt_misc/qemu-aarch64 || true
echo ':qemu-aarch64:M::\x7fELF\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\xb7\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/usr/bin/qemu-aarch64-static:OCF' > /proc/sys/fs/binfmt_misc/register

cd "$1"

branch=${2#"origin/"}

ln -sf "src/arm64" .
ln -sf "src/docker" .
ln -sf "src/compile_tag.sh" .
ln -sf "src/detect_linux_build.sh" .
ln -sf "src/run_builds.sh" .
ln -sf "src/run_arm64_shell.sh" .
ln -sf "src/run_docker_shell.sh" .
chmod +x *.sh

if ./run_builds.sh "$branch" "$3"; then
  rm -f tag_builds/*.zip
  rm -f tag_builds/*.b64
  rm -f tag_builds/*.sha256
  rm -f tag_builds/*.sig
  rm -f tag_builds/*.status

  cd tag_builds/solus/$branch
  ./unittests || exit -1
  exit 0
else
  exit -1
fi
