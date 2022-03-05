#!/bin/bash

CURRENT_DIR=$(pwd)

function create_containers() {
  TYPE=$1
  NAME_EXTRA=
  if [ -z "$TYPE" ]; then
    TYPE=64_bit
  fi

  if [ "$TYPE" = "arm64" ]; then
    for FILE in $(pwd)/arm64/*; do
      chmod +x $FILE
      $FILE 
    done
  else
    for FILE in $(pwd)/docker/${TYPE}/*; do
      DISTRONAME=$(basename ${FILE})
      CONTAINER_NAME=repertory${NAME_EXTRA}_${DISTRONAME}
      TAG_NAME=repertory${NAME_EXTRA}:${DISTRONAME}
      echo Creating Container [${CONTAINER_NAME}]

      docker stop ${CONTAINER_NAME}
      docker rm ${CONTAINER_NAME}
      docker build -t ${TAG_NAME} - < docker/${TYPE}/${DISTRONAME}
      docker stop ${CONTAINER_NAME}
      docker rm ${CONTAINER_NAME}
    done
  fi
}

if [ "$UID" != 0 ]; then
  echo "Must be run as root"
  exit 1
else
  create_containers arm64
  create_containers 64_bit
fi
