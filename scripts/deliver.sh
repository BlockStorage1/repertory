#!/bin/bash

DEST_DIR=$1
DIST_DIR=$2

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$3" "$4" "$5" "$6" "$7"

function error_exit() {
  echo $1
  exit $2
}

if [ ! -d "${PROJECT_DIST_DIR}" ]; then
  error_exit "dist directory not found: ${PROJECT_DIST_DIR}" 2
fi

if [ "${DIST_DIR}" == "" ]; then
  DIST_DIR="${PROJECT_DIST_DIR}"
fi
DIST_DIR=$(realpath "${DIST_DIR}")

if [ ! -d "${DIST_DIR}" ]; then
  error_exit "dist directory not found: ${DIST_DIR}" 2
fi

if [ "${DEST_DIR}" == "" ]; then
  error_exit "dest directory not set" 2
fi

if [ ! -d "${DEST_DIR}" ]; then
  error_exit "dest directory not found: ${DEST_DIR}" 2
fi

pushd "${PROJECT_SOURCE_DIR}"
BRANCH=$(git branch --show-current)
RELEASE=$(grep PROJECT_RELEASE_ITER= ./config.sh | sed s/PROJECT_RELEASE_ITER=//g)
popd

if [ "${BRANCH}" == "master" ] || [ "${BRANCH}" == "alpha" ] ||
  [ "${BRANCH}" == "main" ] || [ "${BRANCH}" == "release" ] ||
  [ "${BRANCH}" == "beta" ] || [ "${BRANCH}" == "rc" ]; then
  DEST_DIR=${DEST_DIR}/${RELEASE}
elif [[ ${BRANCH} = *'-alpha-'* ]] || [[ ${BRANCH} = *'-beta-'* ]] ||
  [[ ${BRANCH} = *'-rc-'* ]] || [[ ${BRANCH} = *'-release-'* ]]; then
  DEST_DIR=${DEST_DIR}/nightly
else
  error_exit "skipping ${PROJECT_FILE_PART}" 0
fi

pushd "${DIST_DIR}"
if [ ! -f "./${PROJECT_OUT_FILE}" ]; then
  error_exit "failed to find file: ${PROJECT_OUT_FILE}" 1
fi

if [ ! -f "./${PROJECT_OUT_FILE}.sha256" ]; then
  error_exit "failed to find file: ${PROJECT_OUT_FILE}.sha256" 1
fi

if [ "${PROJECT_PRIVATE_KEY}" != "" ] && [ ! -f "./${PROJECT_OUT_FILE}.sig" ]; then
  error_exit "failed to find file: ${PROJECT_OUT_FILE}.sig" 1
fi

cp -f ./${PROJECT_OUT_FILE} ${DEST_DIR} || error_exit "failed to deliver file: ${PROJECT_OUT_FILE}" 1
cp -f ./${PROJECT_OUT_FILE}.sha256 ${DEST_DIR} || error_exit "failed to deliver file: ${PROJECT_OUT_FILE}.sha256" 1
if [ "${PROJECT_PRIVATE_KEY}" != "" ]; then
  cp -f ./${PROJECT_OUT_FILE}.sig ${DEST_DIR} || error_exit "failed to deliver file: ${PROJECT_OUT_FILE}.sig" 1
fi
popd

error_exit "delivered ${PROJECT_FILE_PART}" 0
