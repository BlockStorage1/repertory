#!/bin/bash

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$1" "$2" "$3" "$4" "$5"

mkdir -p "${PROJECT_BUILD_DIR}/build"

if [ -d "${PROJECT_DIST_DIR}" ]; then
  rm -rf "${PROJECT_DIST_DIR}"
  mkdir -p "${PROJECT_DIST_DIR}"
fi

if [ -f "${PROJECT_SOURCE_DIR}/pre_build.sh" ]; then
  "${PROJECT_SOURCE_DIR}/pre_build.sh"
fi

ln -sf "${PROJECT_BUILD_DIR}/build/compile_commands.json" "${PROJECT_SOURCE_DIR}/compile_commands.json"

pushd "${PROJECT_BUILD_DIR}"
export CMAKE_BUILD_PARALLEL_LEVEL=${NUM_JOBS}

cmake "${PROJECT_SOURCE_DIR}" \
  -G"Unix Makefiles" \
  -DPROJECT_COMPANY_NAME="${PROJECT_COMPANY_NAME}" \
  -DPROJECT_COPYRIGHT="${PROJECT_COPYRIGHT}" \
  -DPROJECT_DESC="${PROJECT_DESC}" \
  -DPROJECT_INTERFACE=1 \
  -DPROJECT_URL="${PROJECT_URL}" \
  ${PROJECT_CMAKE_OPTS} || exit 1

if [ "${PROJECT_BUILD_CLEAN}" == "clean" ]; then
  make clean
  pushd build
  make clean
  popd
fi

make || exit 1

pushd build
make -j${CMAKE_BUILD_PARALLEL_LEVEL} || exit 1
popd
popd

if [ "${PROJECT_IS_MINGW}" != "1" ] && [ "${PROJECT_REQUIRE_ALPINE}" == "ON" ]; then
  rsync -av --progress /usr/share/icu/74.2/ "${PROJECT_DIST_DIR}/icu/"
fi

if [ "${PROJECT_BUILD_SHARED_LIBS}" == "ON" ] && [ "${PROJECT_IS_MINGW}" != "1" ]; then
  rsync -av --progress ${PROJECT_EXTERNAL_BUILD_ROOT}/ "${PROJECT_DIST_DIR}/"
  pushd "${PROJECT_DIST_DIR}"
  rm lib64
  ln -sf lib lib64
  popd
fi

for APP in ${PROJECT_APP_LIST[@]}; do
  if [ "${PROJECT_BUILD_SHARED_LIBS}" == "ON" ]; then
    if [ "${PROJECT_IS_MINGW}" != "1" ]; then
      rm "${PROJECT_DIST_DIR}/${APP}${PROJECT_APP_BINARY_EXT}"
      if [ "${PROJECT_ENABLE_CURL}" == "ON" ]; then
        mv "${PROJECT_DIST_DIR}/cacert.pem" "${PROJECT_DIST_DIR}/bin/cacert.pem"
      fi
      rsync -av --progress "${PROJECT_BUILD_DIR}/build/${APP}${PROJECT_APP_BINARY_EXT}" "${PROJECT_DIST_DIR}/bin/"
      cat <<EOF >>"${PROJECT_DIST_DIR}/${APP}${PROJECT_APP_BINARY_EXT}"
#!/bin/sh
PROJECT_SCRIPTS_DIR=\$(realpath "\$0")
PROJECT_SCRIPTS_DIR=\$(dirname "\${PROJECT_SCRIPTS_DIR}")

export LD_LIBRARY_PATH="\${PROJECT_SCRIPTS_DIR}/lib:\${PROJECT_SCRIPTS_DIR}/lib64:\${LD_LIBRARY_PATH}"

\${PROJECT_SCRIPTS_DIR}/bin/${APP}${PROJECT_APP_BINARY_EXT} \$*
EOF
      chmod +x "${PROJECT_DIST_DIR}/${APP}${PROJECT_APP_BINARY_EXT}"
    else
      rsync -av --progress "${PROJECT_BUILD_DIR}/build/${APP}${PROJECT_APP_BINARY_EXT}" "${PROJECT_DIST_DIR}/"
    fi
  else
    rsync -av --progress "${PROJECT_BUILD_DIR}/build/${APP}${PROJECT_APP_BINARY_EXT}" "${PROJECT_DIST_DIR}/"
  fi
done

if [ -f "${PROJECT_SOURCE_DIR}/web/${PROJECT_NAME}/pubspec.yaml" ]; then
  rsync -av --progress "${PROJECT_SOURCE_DIR}/web/${PROJECT_NAME}/build/web/" \
    "${PROJECT_DIST_DIR}/web/"
fi

if [ "${PROJECT_IS_MINGW}" == "1" ]; then
  . "${PROJECT_SCRIPTS_DIR}/copy_mingw64_deps.sh" "$1" "$2" "$3" "$4" "$5"
fi

if [ -f "${PROJECT_SOURCE_DIR}/post_build.sh" ]; then
  "${PROJECT_SOURCE_DIR}/post_build.sh"
fi

. "${PROJECT_SCRIPTS_DIR}/make_package.sh" "$1" "$2" "$3" "$4" "$5"
