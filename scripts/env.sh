#!/bin/bash

PROJECT_BUILD_ARCH=$1
PROJECT_CMAKE_BUILD_TYPE=$2
PROJECT_BUILD_CLEAN=$3
PROJECT_IS_MINGW=$4
PROJECT_IS_MINGW_UNIX=$5
DISABLE_CREATE_DIRS=$6

PROJECT_SOURCE_DIR=${PROJECT_SCRIPTS_DIR}/..
PROJECT_SOURCE_DIR=$(realpath "${PROJECT_SOURCE_DIR}")

NUM_JOBS=${MY_NUM_JOBS}
if [[ -z "${NUM_JOBS}" ]]; then
  NUM_JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null ||
    getconf NPROCESSORS_ONLN 2>/dev/null || echo 1)
  if [ "${NUM_JOBS}" -gt "4" ]; then
    NUM_JOBS=$(expr ${NUM_JOBS} - 2)
  elif [ "${NUM_JOBS}" -gt "1" ]; then
    NUM_JOBS=$(expr ${NUM_JOBS} - 1)
  fi
fi

pushd "${PROJECT_SOURCE_DIR}"

PROJECT_GIT_REV=$(git rev-parse --short HEAD)

. "${PROJECT_SCRIPTS_DIR}/versions.sh"
. "${PROJECT_SCRIPTS_DIR}/libraries.sh"

for PROJECT_LIBRARY in "${PROJECT_LIBRARIES[@]}"; do
  ENABLE_NAME=PROJECT_ENABLE_${PROJECT_LIBRARY}
  KEEP_NAME=PROJECT_KEEP_${PROJECT_LIBRARY}
  if [ "${PROJECT_LIBRARY}" == "TESTING" ]; then
    export ${ENABLE_NAME}=ON
  else
    export ${ENABLE_NAME}=OFF
  fi
  export ${KEEP_NAME}=0
done

PROJECT_APP_LIST=()
PROJECT_CMAKE_OPTS=""
PROJECT_ENABLE_WIN32_LONG_PATH_NAMES=OFF
PROJECT_ENABLE_V2_ERRORS=OFF
PROJECT_IS_ALPINE=0
PROJECT_IS_ARM64=0
PROJECT_MINGW64_COPY_DEPENDENCIES=()
PROJECT_MSYS2_PACKAGE_LIST=()
PROJECT_REQUIRE_ALPINE=OFF
PROJECT_STATIC_LINK=OFF

if [ "${PROJECT_BUILD_ARCH}" == "" ]; then
  PROJECT_BUILD_ARCH=x86_64
elif [ "${PROJECT_BUILD_ARCH}" == "aarch64" ]; then
  PROJECT_IS_ARM64=1
fi

if [ "${PROJECT_BUILD_ARCH}" == "x86_64" ]; then
  PROJECT_BUILD_ARCH2="x86-64"
else
  PROJECT_BUILD_ARCH2="${PROJECT_BUILD_ARCH}"
fi

if [ -f /etc/alpine-release ]; then
  PROJECT_IS_ALPINE=1
fi

if [ "${PROJECT_IS_MINGW}" == "" ]; then
  PROJECT_IS_MINGW=0
fi

if [ "${PROJECT_IS_MINGW_UNIX}" == "" ]; then
  PROJECT_IS_MINGW_UNIX=0
fi

. "${PROJECT_SOURCE_DIR}/config.sh"

if [ "${PROJECT_IS_MINGW}" == "0" ]; then
  PROJECT_ENABLE_WIN32_LONG_PATH_NAMES=OFF
fi

if [ "${PROJECT_ENABLE_SFML}" == "ON" ]; then
  PROJECT_ENABLE_FLAC=ON
  PROJECT_ENABLE_FONTCONFIG=ON
  PROJECT_ENABLE_FREETYPE2=ON
  PROJECT_ENABLE_OGG=ON
  PROJECT_ENABLE_OPENAL=ON
  PROJECT_ENABLE_VORBIS=ON
  PROJECT_STATIC_LINK=OFF
fi

if [ "${PROJECT_ENABLE_CPP_HTTPLIB}" == "ON" ]; then
  PROJECT_ENABLE_CURL=ON
  PROJECT_ENABLE_OPENSSL=ON
fi

if [ "${PROJECT_ENABLE_CURL}" == "ON" ]; then
  PROJECT_ENABLE_OPENSSL=ON
fi

if [ "${PROJECT_ENABLE_LIBBITCOIN_SYSTEM}" == "ON" ]; then
  PROJECT_ENABLE_BOOST=ON
  PROJECT_ENABLE_SECP256K1=ON
fi

if [ "${PROJECT_ENABLE_FONTCONFIG}" == "ON" ]; then
  PROJECT_ENABLE_FREETYPE2=ON
fi

if [ "${PROJECT_ENABLE_WXWIDGETS}" == "ON" ]; then
  PROJECT_ENABLE_CURL=ON
  PROJECT_STATIC_LINK=OFF
fi

if [ "${PROJECT_ENABLE_SPDLOG}" == "ON" ]; then
  PROJECT_ENABLE_FMT=OFF
fi

if [ "${PROJECT_ENABLE_VORBIS}" == "ON" ]; then
  PROJECT_ENABLE_OGG=ON
fi

if [ "${PROJECT_ENABLE_FLAC}" == "ON" ]; then
  PROJECT_ENABLE_LIBICONV=ON
  PROJECT_ENABLE_OGG=ON
  PROJECT_ENABLE_VORBIS=ON
fi

if [ "${PROJECT_ENABLE_BOOST}" == "ON" ]; then
  PROJECT_ENABLE_OPENSSL=ON
fi

if [ "${PROJECT_ENABLE_FONTCONFIG}" == "ON" ] || [ "${PROJECT_ENABLE_NANA}" == "ON" ] ||
  [ "${PROJECT_ENABLE_SFML}" == "ON" ] || [ "${PROJECT_ENABLE_WXWIDGETS}" == "ON" ] ||
  [ "${PROJECT_ENABLE_SDL}" == "ON" ]; then
  PROJECT_ENABLE_LIBJPEG_TURBO=ON
  PROJECT_ENABLE_LIBPNG=ON
fi

if [ "${PROJECT_IS_MINGW}" == "1" ]; then
  PROJECT_ENABLE_BACKWARD_CPP=OFF
fi

if [ "${PROJECT_ENABLE_LIBDSM}" == "ON" ]; then
  PROJECT_ENABLE_LIBICONV=ON
  PROJECT_ENABLE_LIBTASN=ON
  PROJECT_ENABLE_OPENSSL=ON
fi

if [ "${PROJECT_CMAKE_BUILD_TYPE}" == "" ]; then
  PROJECT_CMAKE_BUILD_TYPE=RelWithDebInfo
fi

PROJECT_CMAKE_BUILD_TYPE_LOWER=$(echo "${PROJECT_CMAKE_BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')
if [ "${PROJECT_CMAKE_BUILD_TYPE_LOWER}" == "release" ]; then
  PROJECT_CMAKE_BUILD_TYPE=RelWithDebInfo
elif [ "${PROJECT_CMAKE_BUILD_TYPE_LOWER}" == "debug" ]; then
  PROJECT_CMAKE_BUILD_TYPE=Debug
fi

if [ "${PROJECT_IS_MINGW}" == "1" ] && [ "${PROJECT_IS_MINGW_UNIX}" != "1" ]; then
  PROJECT_STATIC_LINK=OFF
fi

if [ "${PROJECT_STATIC_LINK}" == "ON" ]; then
  PROJECT_BUILD_SHARED_LIBS=OFF
  PROJECT_ENABLE_BACKWARD_CPP=OFF
  PROJECT_LINK_TYPE=static
  PROJECT_REQUIRE_ALPINE=ON
else
  PROJECT_BUILD_SHARED_LIBS=ON
  PROJECT_LINK_TYPE=shared
  PROJECT_REQUIRE_ALPINE=OFF
fi

PROJECT_BUILD_DIR=${PROJECT_SOURCE_DIR}/build/${PROJECT_CMAKE_BUILD_TYPE_LOWER}/${PROJECT_LINK_TYPE}
PROJECT_DIST_DIR=${PROJECT_SOURCE_DIR}/dist/${PROJECT_CMAKE_BUILD_TYPE_LOWER}/${PROJECT_LINK_TYPE}

if [ "${PROJECT_IS_MINGW}" == "1" ]; then
  PROJECT_DIST_DIR=${PROJECT_DIST_DIR}/win32
  PROJECT_BUILD_DIR=${PROJECT_BUILD_DIR}/win32
else
  PROJECT_DIST_DIR=${PROJECT_DIST_DIR}/linux
  PROJECT_BUILD_DIR=${PROJECT_BUILD_DIR}/linux
fi

if [ "${PROJECT_IS_ARM64}" == "1" ]; then
  PROJECT_DIST_DIR=${PROJECT_DIST_DIR}/aarch64
  PROJECT_BUILD_DIR=${PROJECT_BUILD_DIR}/aarch64
else
  PROJECT_DIST_DIR=${PROJECT_DIST_DIR}/x86_64
  PROJECT_BUILD_DIR=${PROJECT_BUILD_DIR}/x86_64
fi

PROJECT_DIST_DIR=${PROJECT_DIST_DIR}/${PROJECT_NAME}
PROJECT_EXTERNAL_BUILD_ROOT=${PROJECT_BUILD_DIR}/deps

PROJECT_SUPPORT_DIR=${PROJECT_SOURCE_DIR}/support
PROJECT_3RD_PARTY_DIR=${PROJECT_SUPPORT_DIR}/3rd_party

if [ "${PROJECT_ENABLE_OPENSSL}" == "ON" ]; then
  if [ "${PROJECT_IS_MINGW}" == "1" ]; then
    OPENSSL_ROOT_DIR=/mingw64
  else
    OPENSSL_ROOT_DIR=${PROJECT_EXTERNAL_BUILD_ROOT}
  fi
fi

if [ "${PROJECT_IS_MINGW}" == "1" ] && [ "${PROJECT_IS_MINGW_UNIX}" == "1" ]; then
  PROJECT_TOOLCHAIN_FILE_CMAKE=/cmake_toolchain.cmake
  PROJECT_TOOLCHAIN_FILE_MESON=/meson_cross_file.txt
  PROJECT_CMAKE_OPTS="-DCMAKE_TOOLCHAIN_FILE=${PROJECT_TOOLCHAIN_FILE_CMAKE} ${PROJECT_CMAKE_OPTS}"
fi

if [ -f "${PROJECT_SOURCE_DIR}/cmake/versions.cmake" ]; then
  VERSIONS=($(sed -e s/\ /=/g -e s/set\(//g -e s/\)//g "${PROJECT_SOURCE_DIR}/cmake/versions.cmake"))

  PROJECT_MINGW64_DOCKER_BUILD_ARGS=()

  for VERSION in "${VERSIONS[@]}"; do
    LOOKUP_NAME=$(echo ${VERSION} | sed s/_VERSION.*// | sed s/GTEST/TESTING/g)
    ENABLE_NAME=PROJECT_ENABLE_${LOOKUP_NAME}
    if [ "${!ENABLE_NAME}" != "OFF" ]; then
      PROJECT_MINGW64_DOCKER_BUILD_ARGS+=("--build-arg ${VERSION}")
    fi
  done

  PROJECT_MINGW64_DOCKER_BUILD_ARGS=${PROJECT_MINGW64_DOCKER_BUILD_ARGS[*]}
fi

PROJECT_CMAKE_OPTS="-DPROJECT_3RD_PARTY_DIR=${PROJECT_3RD_PARTY_DIR} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_BUILD_ARCH=${PROJECT_BUILD_ARCH} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_BUILD_DIR=${PROJECT_BUILD_DIR} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_BUILD_SHARED_LIBS=${PROJECT_BUILD_SHARED_LIBS} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_CMAKE_BUILD_TYPE=${PROJECT_CMAKE_BUILD_TYPE} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_DIST_DIR=${PROJECT_DIST_DIR} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_ENABLE_V2_ERRORS=${PROJECT_ENABLE_V2_ERRORS} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_ENABLE_WIN32_LONG_PATH_NAMES=${PROJECT_ENABLE_WIN32_LONG_PATH_NAMES} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_EXTERNAL_BUILD_ROOT=${PROJECT_EXTERNAL_BUILD_ROOT} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_GIT_REV=${PROJECT_GIT_REV} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_IS_ALPINE=${PROJECT_IS_ALPINE} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_IS_ARM64=${PROJECT_IS_ARM64} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_IS_MINGW=${PROJECT_IS_MINGW} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_IS_MINGW_UNIX=${PROJECT_IS_MINGW_UNIX} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_MAJOR_VERSION=${PROJECT_MAJOR_VERSION} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_MINOR_VERSION=${PROJECT_MINOR_VERSION} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_NAME=${PROJECT_NAME} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_RELEASE_ITER=${PROJECT_RELEASE_ITER} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_RELEASE_NUM=${PROJECT_RELEASE_NUM} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_REQUIRE_ALPINE=${PROJECT_REQUIRE_ALPINE} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_REVISION_VERSION=${PROJECT_REVISION_VERSION} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_STATIC_LINK=${PROJECT_STATIC_LINK} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_SUPPORT_DIR=${PROJECT_SUPPORT_DIR} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_TOOLCHAIN_FILE_CMAKE=${PROJECT_TOOLCHAIN_FILE_CMAKE} ${PROJECT_CMAKE_OPTS}"
PROJECT_CMAKE_OPTS="-DPROJECT_TOOLCHAIN_FILE_MESON=${PROJECT_TOOLCHAIN_FILE_MESON} ${PROJECT_CMAKE_OPTS}"

for PROJECT_LIBRARY in "${PROJECT_LIBRARIES[@]}"; do
  ENABLE_NAME=PROJECT_ENABLE_${PROJECT_LIBRARY}
  PROJECT_CMAKE_OPTS="${PROJECT_CMAKE_OPTS} -D${ENABLE_NAME}=${!ENABLE_NAME}"
done

PKG_CONFIG_PATH="${PROJECT_EXTERNAL_BUILD_ROOT}/lib/pkgconfig:${PROJECT_EXTERNAL_BUILD_ROOT}/lib64/pkgconfig:${PROJECT_EXTERNAL_BUILD_ROOT}/shared/pkgconfig:${PKG_CONFIG_PATH}"

if [ "${DISABLE_CREATE_DIRS}" != "1" ]; then
  mkdir -p "${PROJECT_BUILD_DIR}"
  mkdir -p "${PROJECT_DIST_DIR}"
fi

PATH="${PROJECT_EXTERNAL_BUILD_ROOT}/bin:${PATH}"

if [ "${PROJECT_IS_MINGW}" == "1" ]; then
  PROJECT_OS=windows
else
  PROJECT_OS=linux
fi

PROJECT_FILE_PART=${PROJECT_NAME}_${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_REVISION_VERSION}-${PROJECT_RELEASE_ITER}_${PROJECT_GIT_REV}_${PROJECT_OS}_${PROJECT_BUILD_ARCH2}
PROJECT_OUT_FILE=${PROJECT_FILE_PART}.tar.gz

if [ "${PROJECT_IS_MINGW}" == "1" ]; then
  PROJECT_APP_BINARY_EXT=.exe
fi

if [ "${PROJECT_IS_MINGW}" == "1" ] && [ "${PROJECT_IS_MINGW_UNIX}" != "1" ]; then
  MSYS=winsymlinks:nativestrict
fi

export MSYS
export NUM_JOBS
export OPENSSL_ROOT_DIR
export PATH
export PKG_CONFIG_PATH
export PROJECT_3RD_PARTY_DIR
export PROJECT_APP_BINARY_EXT
export PROJECT_APP_LIST
export PROJECT_BUILD_ARCH
export PROJECT_BUILD_ARCH2
export PROJECT_BUILD_CLEAN
export PROJECT_BUILD_DIR
export PROJECT_BUILD_SHARED_LIBS
export PROJECT_CMAKE_BUILD_TYPE
export PROJECT_CMAKE_BUILD_TYPE_LOWER
export PROJECT_CMAKE_OPTS
export PROJECT_COMPANY_NAME
export PROJECT_COPYRIGHT
export PROJECT_DESC
export PROJECT_DIST_DIR
export PROJECT_ENABLE_WIN32_LONG_PATH_NAMES
export PROJECT_ENABLE_V2_ERRORS
export PROJECT_FILE_PART
export PROJECT_GIT_REV
export PROJECT_IS_ALPINE
export PROJECT_IS_ARM64
export PROJECT_IS_MINGW
export PROJECT_IS_MINGW_UNIX
export PROJECT_LINK_TYPE
export PROJECT_MAJOR_VERSION
export PROJECT_MINGW64_COPY_DEPENDENCIES
export PROJECT_MINGW64_DOCKER_BUILD_ARGS
export PROJECT_MINOR_VERSION
export PROJECT_MSYS2_PACKAGE_LIST
export PROJECT_NAME
export PROJECT_OS
export PROJECT_OUT_FILE
export PROJECT_PRIVATE_KEY
export PROJECT_PUBLIC_KEY
export PROJECT_RELEASE_ITER
export PROJECT_RELEASE_NUM
export PROJECT_REQUIRE_ALPINE
export PROJECT_REVISION_VERSION
export PROJECT_SOURCE_DIR
export PROJECT_STATIC_LINK
export PROJECT_SUPPORT_DIR
export PROJECT_TOOLCHAIN_FILE_CMAKE
export PROJECT_TOOLCHAIN_FILE_MESON
export PROJECT_URL

for PROJECT_LIBRARY in "${PROJECT_LIBRARIES[@]}"; do
  ENABLE_NAME=PROJECT_ENABLE_${PROJECT_LIBRARY}
  KEEP_NAME=PROJECT_KEEP_${PROJECT_LIBRARY}
  export ${ENABLE_NAME}
  export ${KEEP_NAME}
done

echo "-=[Settings]=-"
echo "  App binary extension: ${PROJECT_APP_BINARY_EXT}"
echo "  App list: ${PROJECT_APP_LIST[*]}"
echo "  Build arch: ${PROJECT_BUILD_ARCH}"
echo "  Build arch2: ${PROJECT_BUILD_ARCH2}"
echo "  Build clean: ${PROJECT_BUILD_CLEAN}"
echo "  Build dir: ${PROJECT_BUILD_DIR}"
echo "  Build shared libraries: ${PROJECT_BUILD_SHARED_LIBS}"
echo "  CMake options: -G\"Unix Makefiles\" -DPROJECT_COMPANY_NAME=\"${PROJECT_COMPANY_NAME}\" -DPROJECT_COPYRIGHT=\"${PROJECT_COPYRIGHT}\" -DPROJECT_DESC=\"${PROJECT_DESC}\" -DPROJECT_URL=\"${PROJECT_URL}\" ${PROJECT_CMAKE_OPTS} "
echo "  CMake toolchain file: ${PROJECT_TOOLCHAIN_FILE_CMAKE}"
echo "  Cmake Build type: ${PROJECT_CMAKE_BUILD_TYPE}"
echo "  Company name: ${PROJECT_COMPANY_NAME}"
echo "  Copyright: ${PROJECT_COPYRIGHT}"
echo "  Description: ${PROJECT_DESC}"
echo "  Dist dir: ${PROJECT_DIST_DIR}"
echo "  Enable v2 errors: ${PROJECT_ENABLE_V2_ERRORS}"
echo "  External build root: ${PROJECT_EXTERNAL_BUILD_ROOT}"
echo "  File part: ${PROJECT_FILE_PART}"
echo "  Is ARM64: ${PROJECT_IS_ARM64}"
echo "  Is Alpine: ${PROJECT_IS_ALPINE}"
echo "  Is MINGW on Unix: ${PROJECT_IS_MINGW_UNIX}"
echo "  Is MINGW: ${PROJECT_IS_MINGW}"
echo "  Job count: ${NUM_JOBS}"
echo "  Link type: ${PROJECT_LINK_TYPE}"
if [ "${PROJECT_IS_MINGW}" == "1" ]; then
  echo "  Long path names: ${PROJECT_ENABLE_WIN32_LONG_PATH_NAMES}"
fi
echo "  Meson toolchain file: ${PROJECT_TOOLCHAIN_FILE_MESON}"
if [ "${PROJECT_IS_MINGW}" == "1" ] && [ "${PROJECT_IS_MINGW_UNIX}" == "1" ]; then
  echo "  MinGW docker build args: ${PROJECT_MINGW64_DOCKER_BUILD_ARGS}"
fi
echo "  OPENSSL_ROOT_DIR: ${OPENSSL_ROOT_DIR}"
echo "  Out file: ${PROJECT_OUT_FILE}"
echo "  PATH: ${PATH}"
echo "  PKG_CONFIG_PATH: ${PKG_CONFIG_PATH}"
echo "  Require Alpine: ${PROJECT_REQUIRE_ALPINE}"
echo "  Static link: ${PROJECT_STATIC_LINK}"
echo "  Support dir: ${PROJECT_SUPPORT_DIR}"
echo "  Third-party dir: ${PROJECT_3RD_PARTY_DIR}"
echo "  Unit testing enabled: ${PROJECT_ENABLE_TESTING}"
echo "  URL: ${PROJECT_URL}"
echo "-=[Libraries]=-"
for PROJECT_LIBRARY in "${PROJECT_LIBRARIES[@]}"; do
  ENABLE_NAME=PROJECT_ENABLE_${PROJECT_LIBRARY}
  KEEP_NAME=PROJECT_KEEP_${PROJECT_LIBRARY}
  if [ "${!ENABLE_NAME}" == "ON" ] || [ "${!KEEP_NAME}" == "1" ]; then
    echo "  ${ENABLE_NAME}: Enable[${!ENABLE_NAME}] Keep[${!KEEP_NAME}]"
  fi
done
popd
