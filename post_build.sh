#!/bin/bash

CURRENT_DIR=$(dirname "$0")
CURRENT_DIR=$(realpath ${CURRENT_DIR})

rsync -av --progress ${CURRENT_DIR}/${PROJECT_NAME}/${PROJECT_NAME}_test/test_input/ \
  ${PROJECT_BUILD_DIR}/build/test_input/

rsync -av --progress ${CURRENT_DIR}/${PROJECT_NAME}/${PROJECT_NAME}_test/test_input/ \
  ${PROJECT_DIST_DIR}/test_input/

rsync -av --progress ${CURRENT_DIR}/assets/icon.ico ${PROJECT_DIST_DIR}/icon.ico

rsync -av --progress ${CURRENT_DIR}/assets/blue/logo.iconset/icon_128x128.png ${PROJECT_DIST_DIR}/repertory.png
