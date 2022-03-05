#!/bin/bash

NAME=$1
if [ -z "$NAME" ]; then
  echo "Name not set"
elif [ "$UID" != 0 ]; then
  echo "Must be run as root"
  exit 1
else
  TYPE=64_bit

  docker stop repertory_${NAME}
  docker rm repertory_${NAME}
  docker build -t repertory:${NAME} - < docker/${TYPE}/${NAME} && \
    docker run -itd --device /dev/fuse --cap-add SYS_ADMIN --name repertory_${NAME} -v $(pwd):/mnt repertory:${NAME} && \
    docker exec -it repertory_${NAME} /bin/bash
  docker stop repertory_${NAME}
  docker rm repertory_${NAME}
fi
