#!/bin/bash

CMAKE_VERSION=3.22.2

beginsWith() { case $2 in "$1"*) true ;; *) false ;; esac }

TAG=$1
TYPE=$2
# currently unused
ENABLE_UPLOAD=$3

REPOSITORY=repertory
CURRENT_DIR=$(pwd)

TAG_BUILDS_ROOT=${CURRENT_DIR}/tag_builds
mkdir -p "${TAG_BUILDS_ROOT}"

ARM64=0
MACHINE=$(uname -m)
if beginsWith "aarch64" "$MACHINE"; then
  ARM64=1
fi

if beginsWith darwin "$OSTYPE"; then
  PLATFORM_NAME=darwin
else
  PLATFORM_NAME=$(/bin/bash "${CURRENT_DIR}/detect_linux_build.sh")
fi
NUM_JOBS=$(getconf _NPROCESSORS_ONLN 2> /dev/null || getconf NPROCESSORS_ONLN 2> /dev/null || echo 1)

if [ "$PLATFORM_NAME" = "darwin" ]; then
  BASE64_EXEC=base64
  SHA256_EXEC="shasum -a 256 -b"
elif [ "$ARM64" = "1" ]; then
  BASE64_EXEC="base64 -w0"
  SHA256_EXEC="sha256sum -b"
else
  BASE64_EXEC="base64 -w0"
  SHA256_EXEC="sha256sum -b"
fi

if [ -f "${CURRENT_DIR}/src/CMakeLists.txt" ]; then
  REPERTORY_ROOT=${CURRENT_DIR}/src
else
  REPERTORY_ROOT=${CURRENT_DIR}
fi

if [ "$ARM64" = "1" ]; then
  TAG_BUILD=${TAG_BUILDS_ROOT}/arm64_${PLATFORM_NAME}/${TAG}
  ERROR_FILE=${TAG_BUILDS_ROOT}/arm64_${PLATFORM_NAME}.error
else
  TAG_BUILD=${TAG_BUILDS_ROOT}/${PLATFORM_NAME}${NAME_EXTRA}/${TAG}
  ERROR_FILE=${TAG_BUILDS_ROOT}/${PLATFORM_NAME}${NAME_EXTRA}.error
fi
rm -f "${ERROR_FILE}"

split_string() {
  IFS=$1
  DATA=$2
  read -ra SPLIT_STRING <<< "$DATA"
  IFS=' '
}

exit_script() {
  echo "$1" > "${ERROR_FILE}"
  echo "$1"
  cd "${CURRENT_DIR}"
  exit 1
}

set_temp_ld_library_path() {
  LD_LIBRARY_PATH=${TAG_BUILD}/external/lib:${TAG_BUILD}/external/lib64
  export LD_LIBRARY_PATH
}

clear_temp_ld_library_path() {
  LD_LIBRARY_PATH=
  export LD_LIBRARY_PATH
}

if [ "$PLATFORM_NAME" = "unknown" ]; then
  exit_script "Unknown build platform"
elif [ -z "$TAG" ]; then
  exit_script "Branch/tag not supplied"
else
  split_string '_' ${TAG}
  APP_VER=${SPLIT_STRING[0]}

  split_string '-' ${APP_VER}
  APP_RELEASE_TYPE=${SPLIT_STRING[1]}

  split_string '.' ${APP_RELEASE_TYPE}
  APP_RELEASE_TYPE=${SPLIT_STRING[0]}
  APP_RELEASE_TYPE="$(tr '[:lower:]' '[:upper:]' <<< ${APP_RELEASE_TYPE:0:1})${APP_RELEASE_TYPE:1}"
  if [ "$APP_RELEASE_TYPE" = "Rc" ]; then
    APP_RELEASE_TYPE="RC"
  fi
  mkdir -p "${TAG_BUILD}"

  rm -f "${TAG_BUILD}/repertory.sig"
  rm -f "${TAG_BUILD}/repertory.sha256"

  if [ ! -f "${TAG_BUILDS_ROOT}/blockstorage_dev_public.pem" ]; then
    cp -f blockstorage_dev_public.pem "${TAG_BUILDS_ROOT}"
  fi

  if [ ! -f "${TAG_BUILDS_ROOT}/blockstorage_dev_private.pem" ]; then
    cp -f blockstorage_dev_private.pem "${TAG_BUILDS_ROOT}"
  fi

  if [ "$PLATFORM_NAME" = "debian9" ]; then
    CMAKE_OPTS="${CMAKE_OPTS} -DCMAKE_CXX_STANDARD=14"
  fi

  if [ "$ARM64" = "1" ]; then
    CMAKE_OS=linux
    CMAKE_PLATFORM=aarch64
  else
    CMAKE_OS=linux
    CMAKE_PLATFORM=x86_64
  fi

  if [ ! -d "${TAG_BUILDS_ROOT}/cmake-${CMAKE_VERSION}-${CMAKE_OS}-${CMAKE_PLATFORM}" ]; then
    cd "${TAG_BUILDS_ROOT}" || exit_script "Unable to change to directory: ${TAG_BUILDS_ROOT}"
    wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-${CMAKE_OS}-${CMAKE_PLATFORM}.tar.gz || exit_script "Download cmake failed"
    tar xvzf cmake-${CMAKE_VERSION}-${CMAKE_OS}-${CMAKE_PLATFORM}.tar.gz || exit_script "Extract cmake failed"
    rm -f cmake-${CMAKE_VERSION}-${CMAKE_OS}-${CMAKE_PLATFORM}.tar.gz
    cd - || exit_script "Unable to change to directory: -"
  fi
  PATH="${TAG_BUILDS_ROOT}/cmake-${CMAKE_VERSION}-${CMAKE_OS}-${CMAKE_PLATFORM}/bin:$PATH"

  cd "${REPERTORY_ROOT}" || exit_script "Unable to change to directory: ${REPERTORY_ROOT}"
  GIT_REV=$(git rev-parse --short HEAD)
  REPERTORY_VERSION=$(grep set\(REPERTORY_MAJOR ${REPERTORY_ROOT}/CMakeLists.txt | sed s/\)//g | awk '{print $2}')
  REPERTORY_VERSION=${REPERTORY_VERSION}.$(grep set\(REPERTORY_MINOR ${REPERTORY_ROOT}/CMakeLists.txt | sed s/\)//g | awk '{print $2}')
  REPERTORY_VERSION=${REPERTORY_VERSION}.$(grep set\(REPERTORY_REV ${REPERTORY_ROOT}/CMakeLists.txt | sed s/\)//g | awk '{print $2}')
  REPERTORY_VERSION=${REPERTORY_VERSION}-$(grep set\(REPERTORY_RELEASE_ITER ${REPERTORY_ROOT}/CMakeLists.txt | sed s/\)//g | awk '{print $2}')

  if [ "$ARM64" = "1" ]; then
    OUT_FILE=repertory_${REPERTORY_VERSION}_${GIT_REV}_${PLATFORM_NAME}_arm64.zip
  else
    if [ "$PLATFORM_NAME" = "centos7" ]; then
      OUT_FILE=repertory_${REPERTORY_VERSION}_${GIT_REV}_linux_amd64.zip
    else
      OUT_FILE=repertory_${REPERTORY_VERSION}_${GIT_REV}_${PLATFORM_NAME}_amd64.zip
    fi
  fi
  OUT_ZIP=${TAG_BUILDS_ROOT}/${OUT_FILE}

  PATH="${REPERTORY_ROOT}/bin:$PATH"
  export PATH

  if [ -f "${OUT_ZIP}" ]; then
    echo "${PLATFORM_NAME} already exists"
  else
    if [ "${PLATFORM_NAME}" = "tumbleweed" ]; then
      zypper --non-interactive patch
    elif [ "${PLATFORM_NAME}" = "arch" ]; then
      pacman -Syy --noconfirm && pacman -Su --noconfirm
    fi

    cd "${TAG_BUILD}" || exit_script "Unable to change to directory: ${TAG_BUILD}"

    if (cmake "${REPERTORY_ROOT}" ${CMAKE_OPTS} && make -j ${NUM_JOBS}) || (cmake "${REPERTORY_ROOT}" ${CMAKE_OPTS} && make -j ${NUM_JOBS}); then
      PATH=${TAG_BUILD}/external/bin:$PATH
      export PATH

      strip ./repertory
      ${SHA256_EXEC} ./repertory > repertory.sha256 || exit_script "SHA-256 failed for repertory"

      set_temp_ld_library_path
      openssl dgst -sha256 -sign "${TAG_BUILDS_ROOT}/blockstorage_dev_private.pem" -out repertory.sig repertory || exit_script "Create signature failed for repertory"
      openssl dgst -sha256 -verify "${TAG_BUILDS_ROOT}/blockstorage_dev_public.pem" -signature repertory.sig repertory || exit_script "Verify signature failed for repertory"
      clear_temp_ld_library_path

      zip "${OUT_ZIP}" repertory repertory.sha256 repertory.sig cacert.pem || exit_script "Create zip failed"

      cd "${TAG_BUILDS_ROOT}" || exit_script "Unable to change to directory: ${TAG_BUILDS_ROOT}"
      ${SHA256_EXEC} "./${OUT_FILE}" > "${OUT_FILE}.sha256" || exit_script "SHA-256 failed for zip file"

      set_temp_ld_library_path
      openssl dgst -sha256 -sign blockstorage_dev_private.pem -out "${OUT_FILE}.sig" "${OUT_FILE}" || exit_script "Create signature failed for zip file"
      openssl dgst -sha256 -verify blockstorage_dev_public.pem -signature "${OUT_FILE}.sig" "${OUT_FILE}" > "${OUT_FILE}.status" 2>&1 || exit_script "Verify signature failed for zip file"
      clear_temp_ld_library_path

      ${BASE64_EXEC} "${OUT_FILE}.sig" > "${OUT_FILE}.sig.b64" || exit_script "Create base64 signature failed for zip file"
    else
      exit_script "Build failed"
    fi
  fi
  cd "${CURRENT_DIR}"
fi
