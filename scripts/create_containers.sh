#!/bin/bash

pushd "$(dirname "$0")"
CURRENT_DIR=$(pwd)

pushd "${CURRENT_DIR}/.."

function create_containers() {
  TYPE=$1

  for FILE in ./docker/${TYPE}/*; do
    DISTRONAME=$(basename ${FILE})
    CONTAINER_NAME=repertory_${DISTRONAME}
    TAG_NAME=repertory:${DISTRONAME}
    echo Creating Container [${CONTAINER_NAME}]

    docker stop ${CONTAINER_NAME}
    docker rm ${CONTAINER_NAME}
    docker build -t ${TAG_NAME} - < docker/${TYPE}/${DISTRONAME}
    docker stop ${CONTAINER_NAME}
    docker rm ${CONTAINER_NAME}
  done
}

docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
create_containers aarch64
create_containers 64_bit

popd
popd
