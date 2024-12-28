#!/bin/bash

PROJECT_MSYS2_PACKAGE_LIST=()

PROJECT_SCRIPTS_DIR=$(realpath "$0")
PROJECT_SCRIPTS_DIR=$(dirname "${PROJECT_SCRIPTS_DIR}")
. "${PROJECT_SCRIPTS_DIR}/env.sh" "$1" "$2" "$3" "$4" "$5" 1 1>/dev/null 2>&1

PROJECT_MSYS2_PACKAGE_LIST+=(
  mingw64/mingw-w64-x86_64-autotools
  mingw64/mingw-w64-x86_64-clang-tools-extra
  mingw64/mingw-w64-x86_64-cmake
  mingw64/mingw-w64-x86_64-curl
  mingw64/mingw-w64-x86_64-diffutils
  mingw64/mingw-w64-x86_64-gcc
  mingw64/mingw-w64-x86_64-gdb
  mingw64/mingw-w64-x86_64-icu
  mingw64/mingw-w64-x86_64-make
  mingw64/mingw-w64-x86_64-mesa
  mingw64/mingw-w64-x86_64-meson
  mingw64/mingw-w64-x86_64-meson-python
  mingw64/mingw-w64-x86_64-ninja
  mingw64/mingw-w64-x86_64-python
  mingw64/mingw-w64-x86_64-python-mako
  mingw64/mingw-w64-x86_64-toolchain
  mingw64/mingw-w64-x86_64-wget
  mingw64/mingw-w64-x86_64-zlib
  msys/bison
  msys/flex
  msys/git
  msys/patch
  msys/rsync
  make
)

if [ "${PROJECT_ENABLE_BOOST}" == "ON" ]; then
  if [ "${PROJECT_ENABLE_LIBBITCOIN_SYSTEM}" == "ON" ]; then
    yes | pacman -Ry mingw64/mingw-w64-x86_64-boost
  else
    PROJECT_MSYS2_PACKAGE_LIST+=(
      mingw64/mingw-w64-x86_64-boost
    )
  fi
fi

if [ "${PROJECT_ENABLE_CLI11}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-cli11
  )
fi

if [ "${PROJECT_ENABLE_FLAC}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-flac
  )
fi

if [ "${PROJECT_ENABLE_FONTCONFIG}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-fontconfig
  )
fi

if [ "${PROJECT_ENABLE_FREETYPE2}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-freetype
  )
fi

if [ "${PROJECT_ENABLE_LIBDSM}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-libdsm
  )
fi

if [ "${PROJECT_ENABLE_LIBEVENT}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-libevent
  )
fi

if [ "${PROJECT_ENABLE_LIBICONV}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-libiconv
  )
fi

if [ "${PROJECT_ENABLE_LIBJPEG_TURBO}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-libjpeg-turbo
  )
fi

if [ "${PROJECT_ENABLE_LIBPNG}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-libpng
  )
fi

if [ "${PROJECT_ENABLE_LIBSODIUM}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-libsodium
  )
fi

if [ "${PROJECT_ENABLE_LIBTASN}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-libtasn1
  )
fi

if [ "${PROJECT_ENABLE_NANA}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-nana
  )
fi

if [ "${PROJECT_ENABLE_NUSPELL}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-nuspell
  )
fi

if [ "${PROJECT_ENABLE_OGG}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-libogg
  )
fi

if [ "${PROJECT_ENABLE_OPENAL}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-openal
  )
fi

if [ "${PROJECT_ENABLE_OPENSSL}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-openssl
  )
fi

if [ "${PROJECT_ENABLE_PUGIXML}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-pugixml
  )
fi

if [ "${PROJECT_ENABLE_SDL}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-SDL2
  )
fi

if [ "${PROJECT_ENABLE_SFML}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-sfml
  )
fi

if [ "${PROJECT_ENABLE_SQLITE}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-sqlite3
  )
fi

if [ "${PROJECT_ENABLE_TESTING}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-gtest
  )
fi

if [ "${PROJECT_ENABLE_VORBIS}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-libvorbis
  )
fi

if [ "${PROJECT_ENABLE_WXWIDGETS}" == "ON" ]; then
  PROJECT_MSYS2_PACKAGE_LIST+=(
    mingw64/mingw-w64-x86_64-wxwidgets3.2-msw
  )
fi

pacman -Sqyuu --noconfirm &&
  pacman -S --noconfirm --needed --disable-download-timeout msys2-keyring &&
  pacman -S --noconfirm --needed --disable-download-timeout \
    ${PROJECT_MSYS2_PACKAGE_LIST[@]}
