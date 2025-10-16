#!/usr/bin/env bash

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$1" "$2" "$3" "$4" "$5"

if [ -f "${PROJECT_SOURCE_DIR}/web/${PROJECT_NAME}/pubspec.yaml" ]; then
  if [[ "${PROJECT_IS_DARWIN}" == "0" && ("${PROJECT_IS_MINGW}" == "0" || "${PROJECT_IS_MINGW_UNIX}" == "1") ]]; then
    FLUTTER_CONTAINER_NAME="${PROJECT_NAME}_flutter"
    FLUTTER_TAG_NAME="${PROJECT_NAME}:flutter"

    docker stop ${FLUTTER_CONTAINER_NAME}
    docker rm ${FLUTTER_CONTAINER_NAME}

    docker build "${PROJECT_SOURCE_DIR}/docker/x86_64/" \
      --build-arg UID=$(id -u) \
      --build-arg GID=$(id -g) \
      -t ${FLUTTER_TAG_NAME} \
      -f "${PROJECT_SOURCE_DIR}/docker/x86_64/flutter" || exit 1

    docker run -td \
      --name ${FLUTTER_CONTAINER_NAME} \
      -u $(id -u):$(id -g) \
      -v ${PROJECT_SOURCE_DIR}:${PROJECT_SOURCE_DIR}:rw,z \
      -w ${PROJECT_SOURCE_DIR}/web/${PROJECT_NAME} \
      ${FLUTTER_TAG_NAME} || exit 1

    docker exec \
      ${FLUTTER_CONTAINER_NAME} \
      /bin/bash -c \
      "flutter clean;flutter build web --base-href=${PROJECT_FLUTTER_BASE_HREF}" || exit 1

    docker stop ${FLUTTER_CONTAINER_NAME}
    docker rm ${FLUTTER_CONTAINER_NAME}
  else
    pushd "${PROJECT_SOURCE_DIR}/web/${PROJECT_NAME}"
    export MSYS_NO_PATHCONV=1
    flutter clean
    flutter build web --base-href=${PROJECT_FLUTTER_BASE_HREF} || exit 1
    export MSYS_NO_PATHCONV=0
    popd
  fi
fi
