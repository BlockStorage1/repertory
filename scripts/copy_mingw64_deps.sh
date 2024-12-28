#!/bin/bash

PROJECT_MINGW64_COPY_DEPENDENCIES=()

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$1" "$2" "$3" "$4" "$5" 1>/dev/null 2>&1

if [ "${PROJECT_IS_MINGW}" == "1" ] && [ "${PROJECT_STATIC_LINK}" == "OFF" ]; then
  mkdir -p "${PROJECT_DIST_DIR}"
  rm -f ${PROJECT_DIST_DIR}/*.dll

  PROJECT_MINGW64_COPY_DEPENDENCIES+=(
    /mingw64/bin/libgcc_s_seh-1.dll
    /mingw64/bin/libstdc++-6.dll
    /mingw64/bin/libwinpthread-1.dll
    /mingw64/bin/libzlib1.dll
    /mingw64/bin/zlib1.dll
  )

  if [ "${PROJECT_ENABLE_BACKWARD_CPP}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/msvcr90.dll)
  fi

  if [ "${PROJECT_ENABLE_BOOST}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libboost*.dll)
  fi

  if [ "${PROJECT_ENABLE_CLI11}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libcli11*.dll)
  fi

  if [ "${PROJECT_ENABLE_CURL}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(
      /mingw64/bin/libcurl*.dll
      /mingw64/bin/libnghttp2-*.dll
    )
  fi

  if [ "${PROJECT_ENABLE_FLAC}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libFLAC*.dll)
  fi

  if [ "${PROJECT_ENABLE_FMT}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libfmt*.dll)
  fi

  if [ "${PROJECT_ENABLE_FONTCONFIG}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(
      /mingw64/bin/libexpat*.dll
      /mingw64/bin/libfontconfig*.dll
    )
  fi

  if [ "${PROJECT_ENABLE_FREETYPE2}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libfreetype*.dll)
  fi

  if [ "${PROJECT_ENABLE_LIBDSM}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libdsm.dll)
  fi

  if [ "${PROJECT_ENABLE_LIBEVENT}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libevent*.dll)
  fi

  if [ "${PROJECT_ENABLE_LIBICONV}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libiconv.dll)
  fi

  if [ "${PROJECT_ENABLE_LIBJPEG_TURBO}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libjpeg*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libturbo*.dll)
  fi

  if [ "${PROJECT_ENABLE_LIBPNG}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libpng*.dll)
  fi

  if [ "${PROJECT_ENABLE_LIBSODIUM}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libsodium*.dll)
  fi

  if [ "${PROJECT_ENABLE_NANA}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libnana*.dll)
  fi

  if [ "${PROJECT_ENABLE_NUSPELL}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libnuspell-*.dll)
  fi

  if [ "${PROJECT_ENABLE_OGG}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libogg*.dll)
  fi

  if [ "${PROJECT_ENABLE_OPENAL}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libopenal*.dll)
  fi

  if [ "${PROJECT_ENABLE_OPENSSL}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(
      /mingw64/bin/libssl*.dll
      /mingw64/bin/libcrypt*.dll
    )
  fi

  if [ "${PROJECT_ENABLE_PUGIXML}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libpugi*.dll)
  fi

  if [ "${PROJECT_ENABLE_SDL}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/SDL2*.dll)
  fi

  if [ "${PROJECT_ENABLE_SECP256K1}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libsecp256k1*.dll)
  fi

  if [ "${PROJECT_ENABLE_SFML}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libsfml*.dll)
  fi

  if [ "${PROJECT_ENABLE_SPDLOG}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libspdlog*.dll)
  fi

  if [ "${PROJECT_ENABLE_SQLITE}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libsqlite3-*.dll)
  fi

  if [ "${PROJECT_ENABLE_TASN}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libtasn1.dll)
  fi

  if [ "${PROJECT_ENABLE_TESTING}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(
      /mingw64/bin/libgtest*.dll
      /mingw64/bin/libgmock*.dll
    )
  fi

  if [ "${PROJECT_ENABLE_TPL}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libtiny-process*.dll)
  fi

  if [ "${PROJECT_ENABLE_VORBIS}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libvorbis*.dll)
  fi

  if [ "${PROJECT_ENABLE_WXWIDGETS}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libLerc.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libsharpyuv-*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libdeflate.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libglib-2*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libgraphite2.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libharfbuzz-*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libjbig-*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libpcre2-*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libtiff-*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libwebp-*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libwxbase*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/libwxm*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/wxbase*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/bin/wxm*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/lib/gcc_x64_dll/libwxbase*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/lib/gcc_x64_dll/libwxm*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/lib/gcc_x64_dll/wxbase*.dll)
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(/mingw64/lib/gcc_x64_dll/wxm*.dll)
  fi

  if [ "${PROJECT_ENABLE_SDL}" == "ON" ] || [ "${PROJECT_ENABLE_SFML}" == "ON" ] ||
    [ "${PROJECT_ENABLE_WXWIDGETS}" == "ON" ] || [ "${PROJECT_ENABLE_NANA}" == "ON" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(
      /mingw64/bin/libOpenCL*.dll
      /mingw64/bin/libopencl*.dll
      /mingw64/bin/libgallium_wgl.dll
      /mingw64/bin/libva*.dll
      /mingw64/bin/opengl*.dll
      /mingw64/bin/vulkan*.dll
    )
  fi

  rsync -av --progress ${PROJECT_EXTERNAL_BUILD_ROOT}/bin/*.dll "${PROJECT_DIST_DIR}/"
  rsync -av --progress ${PROJECT_EXTERNAL_BUILD_ROOT}/lib/*.dll "${PROJECT_DIST_DIR}/"
  if [ "${PROJECT_ENABLE_WXWIDGETS}" == "ON" ]; then
    rsync -av --progress ${PROJECT_EXTERNAL_BUILD_ROOT}/lib/gcc_x64_dll/*.dll "${PROJECT_DIST_DIR}/"
  fi
fi

if [ "${PROJECT_ENABLE_WINFSP}" == "ON" ]; then
  if [ "${PROJECT_BUILD_ARCH}" == "x86_64" ]; then
    WINFSP_DLL_PART=x64
  fi

  if [ "${WINFSP_DLL_PART}" != "" ]; then
    PROJECT_MINGW64_COPY_DEPENDENCIES+=(${PROJECT_3RD_PARTY_DIR}/winfsp-2.0/bin/winfsp-${WINFSP_DLL_PART}.dll)
  fi
fi

if [ "${PROJECT_ENABLE_VLC}" == "ON" ]; then
  rsync -av --progress ${PROJECT_3RD_PARTY_DIR}/vlc/ "${PROJECT_DIST_DIR}/vlc/"
fi

for PROJECT_DEPENDENCY in "${PROJECT_MINGW64_COPY_DEPENDENCIES[@]}"; do
  rsync -av --progress ${PROJECT_DEPENDENCY} "${PROJECT_DIST_DIR}/"
done
