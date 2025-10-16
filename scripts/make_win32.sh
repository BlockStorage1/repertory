#!/usr/bin/env bash

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$1" "$2" "$3" 1 1

if [ -f "${PROJECT_SCRIPTS_DIR}/cleanup.sh" ]; then
  . "${PROJECT_SCRIPTS_DIR}/cleanup.sh" "$1" "$2" "$3" 1 1
  rm ${PROJECT_SCRIPTS_DIR}/cleanup.*
fi

DOCKER_NAME=mingw64
DOCKER_CONTAINER=${PROJECT_NAME}_${DOCKER_NAME}
DOCKER_TAG=${PROJECT_NAME}:${DOCKER_NAME}

. "${PROJECT_SCRIPTS_DIR}/docker_common.sh" || exit 1
