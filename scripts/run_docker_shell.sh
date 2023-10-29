#!/bin/bash

pushd "$(dirname "$0")"
CURRENT_DIR=$(pwd)

pushd "${CURRENT_DIR}/.."
NAME=$1
TYPE=$2
if [ -z "$NAME" ]; then
  echo "Name not set"
else
  if [ -z "${TYPE}" ]; then
    TYPE=64_bit
  fi

  docker stop repertory_${NAME}
  docker rm repertory_${NAME}
  docker build -t repertory:${NAME} - < docker/${TYPE}/${NAME} &&
    docker run -itd --device /dev/fuse --cap-add SYS_ADMIN --name repertory_${NAME} -v $(pwd):/mnt repertory:${NAME} &&
    docker exec -it repertory_${NAME} /bin/bash
  docker stop repertory_${NAME}
  docker rm repertory_${NAME}
fi
popd
popd
