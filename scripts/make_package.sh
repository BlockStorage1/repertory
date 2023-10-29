#!/bin/bash

PLATFORM_NAME=$1
SIGNING_FOLDER=$2
BINARY_FOLDER=$3
SOURCE_FOLDER=$4
OUTPUT_FOLDER=$5
ARM64=$6

LINUX_PLATFORM=alpine

exit_script() {
  echo "$1"
  cd "${CURRENT_DIR}"
  exit 1
}

if [ -z "${PLATFORM_NAME}" ]; then
  exit_script "'PLATFORM_NAME' is not set (arg1)"
fi

if [ -z "${SIGNING_FOLDER}" ]; then
  exit_script "'SIGNING_FOLDER' is not set (arg2)"
fi

if [ -z "${BINARY_FOLDER}" ]; then
  exit_script "'BINARY_FOLDER' is not set (arg3)"
fi

if [ -z "${SOURCE_FOLDER}" ]; then
  exit_script "'SOURCE_FOLDER' is not set (arg4)"
fi

if [ -z "${OUTPUT_FOLDER}" ]; then
  exit_script "'OUTPUT_FOLDER' is not set (arg5)"
fi

if [ "${PLATFORM_NAME}" == "darwin" ]; then
  SHA256_EXEC="shasum -a 256 -b"
else
  SHA256_EXEC="sha256sum -b"
fi

pushd "${SOURCE_FOLDER}"
BRANCH=$(git branch --show-current)
RELEASE=$(grep set\(REPERTORY_RELEASE_ITER ./CMakeLists.txt | sed s/\)//g | awk '{print $2}')

if [ "${BRANCH}" == "master" ] || [ "${BRANCH}" == "alpha" ] || [ "${BRANCH}" == "beta" ] || [ "${BRANCH}" == "rc" ]; then
  OUTPUT_FOLDER=${OUTPUT_FOLDER}/${RELEASE}
else
  OUTPUT_FOLDER=${OUTPUT_FOLDER}/nightly
fi

GIT_REV=$(git rev-parse --short HEAD)
REPERTORY_VERSION=$(grep set\(REPERTORY_MAJOR ./CMakeLists.txt | sed s/\)//g | awk '{print $2}')
REPERTORY_VERSION=${REPERTORY_VERSION}.$(grep set\(REPERTORY_MINOR ./CMakeLists.txt | sed s/\)//g | awk '{print $2}')
REPERTORY_VERSION=${REPERTORY_VERSION}.$(grep set\(REPERTORY_REV ./CMakeLists.txt | sed s/\)//g | awk '{print $2}')
REPERTORY_VERSION=${REPERTORY_VERSION}-${RELEASE}
popd

REPERTORY_BINARY=repertory
if [ "${ARM64}" == "1" ]; then
  if [ "${PLATFORM_NAME}" == "${LINUX_PLATFORM}" ]; then
    ZIP_FILE_NAME=repertory_${REPERTORY_VERSION}_${GIT_REV}_linux_arm64.zip
  else
    ZIP_FILE_NAME=repertory_${REPERTORY_VERSION}_${GIT_REV}_${PLATFORM_NAME}_arm64.zip
  fi
elif [ "${PLATFORM_NAME}" == "${LINUX_PLATFORM}" ]; then
  ZIP_FILE_NAME=repertory_${REPERTORY_VERSION}_${GIT_REV}_linux_amd64.zip
elif [ "${PLATFORM_NAME}" == "mingw64" ]; then
  ZIP_FILE_NAME=repertory_${REPERTORY_VERSION}_${GIT_REV}_windows_amd64.zip
  REPERTORY_BINARY=${REPERTORY_BINARY}.exe
else
  ZIP_FILE_NAME=repertory_${REPERTORY_VERSION}_${GIT_REV}_${PLATFORM_NAME}_amd64.zip
fi
ZIP_FILE_PATH=./${ZIP_FILE_NAME}

create_hash_and_sig() {
  ${SHA256_EXEC} ./${1} > ${1}.sha256 || exit_script "SHA-256 failed for $1"
  openssl dgst -sha256 -sign "${SIGNING_FOLDER}/blockstorage_dev_private.pem" -out ${1}.sig $1 || exit_script "Create signature failed for $1"
  openssl dgst -sha256 -verify "${SIGNING_FOLDER}/blockstorage_dev_public.pem" -signature ${1}.sig $1 || exit_script "Verify signature failed for $1"
}

clean_directory() {
  rm -f "${ZIP_FILE_PATH}"
  rm -f "${ZIP_FILE_PATH}.sha256"
  rm -f "${ZIP_FILE_PATH}.sig"
  rm -f "cacert.pem.sha256"
  rm -f "cacert.pem.sig"
  rm -f "${REPERTORY_BINARY}.sha256"
  rm -f "${REPERTORY_BINARY}.sig"
  if [ "${PLATFORM_NAME}" == "mingw64" ]; then
    rm -f "winfsp-x64.dll.sha256"
    rm -f "winfsp-x64.dll.sig"
  fi
}

pushd "${BINARY_FOLDER}"
clean_directory

strip ./${REPERTORY_BINARY}

create_hash_and_sig ${REPERTORY_BINARY}
create_hash_and_sig cacert.pem
if [ "${PLATFORM_NAME}" == "mingw64" ]; then
  create_hash_and_sig winfsp-x64.dll
fi

if [ "${PLATFORM_NAME}" == "mingw64" ]; then
  zip "${ZIP_FILE_PATH}" \
    ${REPERTORY_BINARY} ${REPERTORY_BINARY}.sha256 ${REPERTORY_BINARY}.sig \
    winfsp-x64.dll winfsp-x64.dll.sha256 winfsp-x64.dll.sig \
    cacert.pem cacert.pem.sha256 cacert.pem.sig || exit_script "Create zip failed"
else
  zip "${ZIP_FILE_PATH}" \
    ${REPERTORY_BINARY} ${REPERTORY_BINARY}.sha256 ${REPERTORY_BINARY}.sig \
    cacert.pem cacert.pem.sha256 cacert.pem.sig || exit_script "Create zip failed"
fi

create_hash_and_sig ${ZIP_FILE_NAME}

cp -f "${ZIP_FILE_PATH}" "${OUTPUT_FOLDER}" || exit_script "Copy ${ZIP_FILE_NAME} failed"
cp -f "${ZIP_FILE_PATH}.sha256" "${OUTPUT_FOLDER}" || exit_script "Copy ${ZIP_FILE_NAME}.sha256 failed"
cp -f "${ZIP_FILE_PATH}.sig" "${OUTPUT_FOLDER}" || exit_script "Copy ${ZIP_FILE_NAME}.sig failed"

clean_directory
popd
