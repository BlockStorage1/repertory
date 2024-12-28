#!/bin/bash

TEMP_DIR=$(mktemp -d)

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$1" "$2" "$3" "$4" "$5"

function error_exit() {
  echo $1
  rm -rf ${TEMP_DIR}
  exit $2
}

function create_file_validations() {
  local SOURCE_FILE=$1
  sha256sum ${SOURCE_FILE} >${SOURCE_FILE}.sha256 || error_exit "failed to create sha256 for file: ${SOURCE_FILE}" 1
  if [ "${PROJECT_PRIVATE_KEY}" != "" ]; then
    openssl dgst -sha256 -sign "${PROJECT_PRIVATE_KEY}" -out "${SOURCE_FILE}.sig" "${SOURCE_FILE}" || error_exit "failed to create signature for file: ${SOURCE_FILE}" 1
    openssl dgst -sha256 -verify "${PROJECT_PUBLIC_KEY}" -signature "${SOURCE_FILE}.sig" "${SOURCE_FILE}" || error_exit "failed to validate signature for file: ${SOURCE_FILE}" 1
  fi
}

if [ ! -d "${PROJECT_DIST_DIR}" ]; then
  error_exit "dist directory not found: ${PROJECT_DIST_DIR}" 2
fi

pushd "${PROJECT_DIST_DIR}"
if [ -f "${PROJECT_OUT_FILE}" ]; then
  rm -f "${PROJECT_OUT_FILE}" || error_exit "failed to delete file: ${PROJECT_OUT_FILE}" 1
fi
if [ -f "${PROJECT_OUT_FILE}.sha256" ]; then
  rm -f "${PROJECT_OUT_FILE}.sha256" || error_exit "failed to delete file: ${PROJECT_OUT_FILE}.sha256" 1
fi
if [ -f "${PROJECT_OUT_FILE}.sig" ]; then
  rm -f "${PROJECT_OUT_FILE}.sig" || error_exit "failed to delete file: ${PROJECT_OUT_FILE}.sig" 1
fi
popd

rsync -av --progress ${PROJECT_DIST_DIR}/ ${TEMP_DIR}/${PROJECT_NAME}/ || error_exit "failed to rsync" 1

for APP in ${PROJECT_APP_LIST[@]}; do
  if [ "${PROJECT_BUILD_SHARED_LIBS}" == "ON" ] && [ "${PROJECT_IS_MINGW}" != "1" ]; then
    strip ${TEMP_DIR}/${PROJECT_NAME}/bin/${APP}${PROJECT_APP_BINARY_EXT}
  else
    strip ${TEMP_DIR}/${PROJECT_NAME}/${APP}${PROJECT_APP_BINARY_EXT}
  fi
done

pushd "${TEMP_DIR}/${PROJECT_NAME}/"
IFS=$'\n'
set -f
FILE_LIST=$(find . -type f)
for FILE in ${FILE_LIST}; do
  create_file_validations "${FILE}"
done
unset IFS
set +f
popd

pushd "${PROJECT_DIST_DIR}"
tar cvzf "${PROJECT_OUT_FILE}" -C ${TEMP_DIR} . || error_exit "failed to create archive: ${PROJECT_OUT_FILE}" 1
create_file_validations "${PROJECT_OUT_FILE}"
popd

error_exit "created package ${PROJECT_FILE_PART}" 0
