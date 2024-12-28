#!/bin/bash

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$1" "$2" "$3" "$4" "$5" 0

pushd "${PROJECT_DIST_DIR}"
if [ -e "./${PROJECT_NAME}_test${PROJECT_APP_BINARY_EXT}" ]; then
  for APP in ${PROJECT_APP_LIST[@]}; do
    if [[ "$APP" == *_test ]]; then
      if [ "${PROJECT_IS_MINGW_UNIX}" == "1" ]; then
        wine64 "${PROJECT_DIST_DIR}/${PROJECT_NAME}_test${PROJECT_APP_BINARY_EXT}" || exit 1
      else
        "./${PROJECT_NAME}_test${PROJECT_APP_BINARY_EXT}" || exit 1
      fi
    fi
  done
fi
popd
