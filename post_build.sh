#!/bin/bash

CURRENT_DIR=$(dirname "$0")
CURRENT_DIR=$(realpath ${CURRENT_DIR})

rsync -av --progress ${CURRENT_DIR}/${PROJECT_NAME}/${PROJECT_NAME}_test/test_input/ \
  ${PROJECT_BUILD_DIR}/build/test_input/

rsync -av --progress ${CURRENT_DIR}/${PROJECT_NAME}/${PROJECT_NAME}_test/test_input/ \
  ${PROJECT_DIST_DIR}/test_input/
