#!/bin/bash

DOCKER_NAME=$1

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$2" "$3" "$4" "$5" "$6" 1 1>/dev/null 2>&1

if [ -f "${PROJECT_SCRIPTS_DIR}/cleanup.sh" ]; then
  . "${PROJECT_SCRIPTS_DIR}/cleanup.sh" "$2" "$3" "$4" "$5" "$6"
  rm ${PROJECT_SCRIPTS_DIR}/cleanup.*
fi

DOCKER_CONTAINER=${PROJECT_NAME}_${DOCKER_NAME}_${PROJECT_BUILD_ARCH}_shell
DOCKER_TAG=${PROJECT_NAME}:${DOCKER_NAME}
DOCKER_SHELL=1

. "${PROJECT_SCRIPTS_DIR}/docker_common.sh" || exit 1
