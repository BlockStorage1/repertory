#!/bin/bash

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$1" "$2" "$3" 0 0

if [ -f "${PROJECT_SCRIPTS_DIR}/cleanup.sh" ]; then
  . "${PROJECT_SCRIPTS_DIR}/cleanup.sh" "$1" "$2" "$3" 0 0
  rm ${PROJECT_SCRIPTS_DIR}/cleanup.*
fi

"${PROJECT_SOURCE_DIR}/scripts/make_flutter.sh" "${PROJECT_BUILD_ARCH}" "${PROJECT_CMAKE_BUILD_TYPE}" "${PROJECT_BUILD_CLEAN}" 0 0 || exit 1

if [ "${PROJECT_REQUIRE_ALPINE}" == "ON" ] || [ "${PROJECT_IS_ARM64}" == "1" ]; then
  DOCKER_NAME=alpine
  DOCKER_CONTAINER=${PROJECT_NAME}_${DOCKER_NAME}_${PROJECT_BUILD_ARCH}
  DOCKER_TAG=${PROJECT_NAME}:${DOCKER_NAME}

  . "${PROJECT_SCRIPTS_DIR}/docker_common.sh" || exit 1
else
  "${PROJECT_SOURCE_DIR}/scripts/make_common.sh" "${PROJECT_BUILD_ARCH}" "${PROJECT_CMAKE_BUILD_TYPE}" "${PROJECT_BUILD_CLEAN}" 0 0 || exit 1
fi
