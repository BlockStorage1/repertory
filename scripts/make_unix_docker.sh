#!/bin/bash

BUILD_TYPE=$1
BUILD_CLEAN=$2
IS_MINGW=$3
if [ "${IS_MINGW}" == "1" ]; then
  BUILD_ROOT=build3
else
  BUILD_ROOT=build
fi

BUILD_FOLDER=$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')
if [ "${BUILD_FOLDER}" == "release" ]; then
  BUILD_TYPE=Release
fi

NUM_JOBS=${MY_NUM_JOBS}
if [[ -z "${NUM_JOBS}" ]]; then
  NUM_JOBS=$(getconf _NPROCESSORS_ONLN 2> /dev/null || getconf NPROCESSORS_ONLN 2> /dev/null || echo 1)
  if [ "${NUM_JOBS}" -gt "2" ]; then
    NUM_JOBS=$(expr ${NUM_JOBS} - 2)
  elif [ "${NUM_JOBS}" -gt "1" ]; then
    NUM_JOBS=$(expr ${NUM_JOBS} - 1)
  fi
fi
echo "Job count: ${NUM_JOBS}"

pushd "$(dirname "$0")"
mkdir -p ../${BUILD_ROOT}/${BUILD_FOLDER}

pushd ../${BUILD_ROOT}/${BUILD_FOLDER}

if [ "${IS_MINGW}" == "1" ]; then
  TOOLCHAIN=$(realpath ../../cmake/mingw-w64-x86_64.cmake)
  EXE_EXT=.exe
  CMAKE_ADDITIONAL_OPTS=-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN}
fi

cmake ../.. ${CMAKE_ADDITIONAL_OPTS} \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DREPERTORY_ENABLE_S3=ON \
  -DREPERTORY_ENABLE_S3_TESTING=ON || exit 1

pushd ..
ln -sf ${BUILD_FOLDER}/compile_commands.json .
ln -sf ${BUILD_FOLDER}/repertory${EXE_EXT} .
ln -sf ${BUILD_FOLDER}/unittests${EXE_EXT} .
if [ "${IS_MINGW}" == "1" ]; then
  ln -sf ${BUILD_FOLDER}/winfsp-x64.dll .
fi
popd

if [ "${BUILD_CLEAN}" == "clean" ]; then
  make clean
fi

make -j${NUM_JOBS} || exit 1
popd

pushd ../${BUILD_ROOT}
ln -sf ${BUILD_FOLDER}/compile_commands.json .
ln -sf ${BUILD_FOLDER}/repertory${EXE_EXT} .
ln -sf ${BUILD_FOLDER}/unittests${EXE_EXT} .
if [ "${IS_MINGW}" == "1" ]; then
  ln -sf ${BUILD_FOLDER}/winfsp-x64.dll .
fi
popd

ln -sf ../3rd_party/cacert.pem ../${BUILD_ROOT}/cacert.pem
popd
