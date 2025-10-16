#!/usr/bin/env bash

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$1" "$2" "$3" "$4" "$5" 1 1>/dev/null 2>&1

if [ -f "${PROJECT_SCRIPTS_DIR}/cleanup.sh" ]; then
  . "${PROJECT_SCRIPTS_DIR}/cleanup.sh" "$1" "$2" "$3" "$4" "$5"
  rm ${PROJECT_SCRIPTS_DIR}/cleanup.*
fi

function create_containers() {
  BUILD_TYPE=$1

  for FILE in "${PROJECT_SOURCE_DIR}/docker/${BUILD_TYPE}/*"; do
    DOCKER_CREATE_ONLY=1
    DOCKER_NAME=$(basename ${FILE})
    DOCKER_TAG=${PROJECT_NAME}:${DOCKER_NAME}

    . "${PROJECT_SCRIPTS_DIR}/docker_common.sh"
  done
}

create_containers aarch64
create_containers x86_64
