#!/bin/bash

BUILD_TYPE=$1
BUILD_CLEAN=$2
BUILD_ARCH=$3

SOURCE_DIR=$(dirname "$0")/..
SOURCE_DIR=$(realpath ${SOURCE_DIR})

NAME=alpine
if [ -z "${BUILD_ARCH}" ]; then
  BUILD_ARCH=64_bit
fi

docker stop repertory_${NAME}
docker rm repertory_${NAME}
docker build -t repertory:${NAME} - < ${SOURCE_DIR}/docker/${BUILD_ARCH}/${NAME} &&
  docker run -td -u $(id -u):$(id -g) --device /dev/fuse --cap-add SYS_ADMIN --name repertory_${NAME} --env MY_NUM_JOBS=${MY_NUM_JOBS} -w ${SOURCE_DIR} -v ${SOURCE_DIR}:${SOURCE_DIR}:rw,z repertory:${NAME} &&
  docker exec repertory_${NAME} /bin/bash -c "${SOURCE_DIR}/scripts/make_unix_docker.sh ${BUILD_TYPE} ${BUILD_CLEAN}"
docker stop repertory_${NAME}
docker rm repertory_${NAME}
