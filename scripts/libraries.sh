#!/usr/bin/env bash

PROJECT_LIBRARIES=(
  BOOST
  CPP_HTTPLIB
  CURL
  FUSE
  JSON
  LIBSODIUM
  OPENSSL
  PUGIXML
  ROCKSDB
  SPDLOG
  SQLITE
  STDUUID
  TESTING
  WINFSP
)

declare -A PROJECT_CLEANUP
PROJECT_CLEANUP[BINUTILS]="3rd_party/mingw64/binutils-*"
PROJECT_CLEANUP[BOOST]="3rd_party/boost_*"
PROJECT_CLEANUP[CPP_HTTPLIB]="3rd_party/cpp-httplib-*"
PROJECT_CLEANUP[CURL]="3rd_party/curl-*"
PROJECT_CLEANUP[EXPAT]="3rd_party/mingw64/expat-*"
PROJECT_CLEANUP[GCC]="3rd_party/mingw64/gcc-*"
PROJECT_CLEANUP[ICU]="3rd_party/mingw64/icu-release-*"
PROJECT_CLEANUP[JSON]="3rd_party/json-*"
PROJECT_CLEANUP[INNOSETUP]="3rd_party/mingw64/innosetup-*"
PROJECT_CLEANUP[LIBBITCOIN_SYSTEM_ON]="3rd_party/boost_${PROJECT_VERSIONS[BOOST_MAJOR]}_${PROJECT_VERSIONS[BOOST_MINOR]}_*"
PROJECT_CLEANUP[LIBSODIUM]="3rd_party/libsodium-*:3rd_party/libsodium*"
PROJECT_CLEANUP[MINGW]="3rd_party/mingw64/mingw-w64-*"
PROJECT_CLEANUP[OPENSSL]="3rd_party/openssl-*"
PROJECT_CLEANUP[PKG_CONFIG]="3rd_party/mingw64/pkg-config-*"
PROJECT_CLEANUP[PUGIXML]="3rd_party/pugixml-*"
PROJECT_CLEANUP[ROCKSDB]="3rd_party/rocksdb-*"
PROJECT_CLEANUP[SPDLOG]="3rd_party/spdlog-*"
PROJECT_CLEANUP[SQLITE]="3rd_party/sqlite*"
PROJECT_CLEANUP[STDUUID]="3rd_party/stduuid-*"
PROJECT_CLEANUP[TESTING]="3rd_party/googletest-*"
PROJECT_CLEANUP[WINFSP]="3rd_party/winfsp-*"
PROJECT_CLEANUP[ZLIB]="3rd_party/mingw64/zlib-*"
export PROJECT_CLEANUP

declare -A PROJECT_DOWNLOADS
PROJECT_DOWNLOADS[BINUTILS]="https://ftp.gnu.org/gnu/binutils/binutils-${PROJECT_VERSIONS[BINUTILS]}.tar.xz;binutils-${PROJECT_VERSIONS[BINUTILS]}.tar.xz;3rd_party/mingw64"
PROJECT_DOWNLOADS[BOOST2]="https://archives.boost.io/release/${PROJECT_VERSIONS[BOOST2_MAJOR]}.${PROJECT_VERSIONS[BOOST2_MINOR]}.${PROJECT_VERSIONS[BOOST2_PATCH]}/source/boost_${PROJECT_VERSIONS[BOOST2_MAJOR]}_${PROJECT_VERSIONS[BOOST2_MINOR]}_${PROJECT_VERSIONS[BOOST2_PATCH]}.tar.gz;boost_${PROJECT_VERSIONS[BOOST2_MAJOR]}_${PROJECT_VERSIONS[BOOST2_MINOR]}_${PROJECT_VERSIONS[BOOST2_PATCH]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[BOOST]="https://archives.boost.io/release/${PROJECT_VERSIONS[BOOST_MAJOR]}.${PROJECT_VERSIONS[BOOST_MINOR]}.${PROJECT_VERSIONS[BOOST_PATCH]}/source/boost_${PROJECT_VERSIONS[BOOST_MAJOR]}_${PROJECT_VERSIONS[BOOST_MINOR]}_${PROJECT_VERSIONS[BOOST_PATCH]}.tar.gz;boost_${PROJECT_VERSIONS[BOOST_MAJOR]}_${PROJECT_VERSIONS[BOOST_MINOR]}_${PROJECT_VERSIONS[BOOST_PATCH]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[CPP_HTTPLIB]="https://github.com/yhirose/cpp-httplib/archive/refs/tags/v${PROJECT_VERSIONS[CPP_HTTPLIB]}.tar.gz;cpp-httplib-${PROJECT_VERSIONS[CPP_HTTPLIB]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[CURL]="https://github.com/curl/curl/archive/refs/tags/curl-${PROJECT_VERSIONS[CURL2]}.tar.gz;curl-${PROJECT_VERSIONS[CURL]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[EXPAT]="https://github.com/libexpat/libexpat/archive/refs/tags/R_${PROJECT_VERSIONS[EXPAT2]}.tar.gz;expat-${PROJECT_VERSIONS[EXPAT]}.tar.gz;3rd_party/mingw64"
PROJECT_DOWNLOADS[GCC]="https://mirrorservice.org/sites/sourceware.org/pub/gcc/releases/gcc-${PROJECT_VERSIONS[GCC]}/gcc-${PROJECT_VERSIONS[GCC]}.tar.gz;gcc-${PROJECT_VERSIONS[GCC]}.tar.gz;3rd_party/mingw64"
PROJECT_DOWNLOADS[GTEST]="https://github.com/google/googletest/archive/refs/tags/v${PROJECT_VERSIONS[GTEST]}.tar.gz;googletest-${PROJECT_VERSIONS[GTEST]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[ICU]="https://github.com/unicode-org/icu/archive/refs/tags/release-${PROJECT_VERSIONS[ICU]}.tar.gz;icu-release-${PROJECT_VERSIONS[ICU]}.tar.gz;3rd_party/mingw64"
PROJECT_DOWNLOADS[JSON]="https://github.com/nlohmann/json/archive/refs/tags/v${PROJECT_VERSIONS[JSON]}.tar.gz;json-${PROJECT_VERSIONS[JSON]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[INNOSETUP]="https://files.jrsoftware.org/is/6/innosetup-${PROJECT_VERSIONS[INNOSETUP]}.exe;innosetup-${PROJECT_VERSIONS[INNOSETUP]}.exe;3rd_party/mingw64"
PROJECT_DOWNLOADS[WINFSP]="https://github.com/winfsp/winfsp/releases/download/v${PROJECT_VERSIONS[WINFSP2]}/winfsp-${PROJECT_VERSIONS[WINFSP]}.msi;winfsp-${PROJECT_VERSIONS[WINFSP]}.msi;3rd_party"
PROJECT_DOWNLOADS[LIBSODIUM]="https://github.com/jedisct1/libsodium/archive/refs/tags/${PROJECT_VERSIONS[LIBSODIUM]}-RELEASE.tar.gz;libsodium-${PROJECT_VERSIONS[LIBSODIUM]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[MINGW]="https://sourceforge.net/projects/mingw-w64/files/mingw-w64/mingw-w64-release/mingw-w64-v${PROJECT_VERSIONS[MINGW]}.tar.bz2;;mingw-w64-v${PROJECT_VERSIONS[MINGW]}.tar.bz2;3rd_party/mingw64"
PROJECT_DOWNLOADS[OPENSSL]="https://github.com/openssl/openssl/releases/download/openssl-${PROJECT_VERSIONS[OPENSSL]}/openssl-${PROJECT_VERSIONS[OPENSSL]}.tar.gz;openssl-${PROJECT_VERSIONS[OPENSSL]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[PKG_CONFIG]="https://pkgconfig.freedesktop.org/releases/pkg-config-${PROJECT_VERSIONS[PKG_CONFIG]}.tar.gz;pkg-config-${PROJECT_VERSIONS[PKG_CONFIG]}.tar.gz;3rd_party/mingw64"
PROJECT_DOWNLOADS[PUGIXML]="https://github.com/zeux/pugixml/releases/download/v${PROJECT_VERSIONS[PUGIXML]}/pugixml-${PROJECT_VERSIONS[PUGIXML]}.tar.gz;pugixml-${PROJECT_VERSIONS[PUGIXML]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[ROCKSDB]="https://github.com/facebook/rocksdb/archive/refs/tags/v${PROJECT_VERSIONS[ROCKSDB]}.tar.gz;rocksdb-${PROJECT_VERSIONS[ROCKSDB]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[SPDLOG]="https://github.com/gabime/spdlog/archive/refs/tags/v${PROJECT_VERSIONS[SPDLOG]}.tar.gz;spdlog-${PROJECT_VERSIONS[SPDLOG]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[SQLITE]="https://www.sqlite.org/2025/sqlite-amalgamation-${PROJECT_VERSIONS[SQLITE]}.zip;sqlite-amalgamation-${PROJECT_VERSIONS[SQLITE]}.zip;3rd_party"
PROJECT_DOWNLOADS[STDUUID]="https://github.com/mariusbancila/stduuid/archive/refs/tags/v${PROJECT_VERSIONS[STDUUID]}.tar.gz;stduuid-${PROJECT_VERSIONS[STDUUID]}.tar.gz;3rd_party"
PROJECT_DOWNLOADS[ZLIB]="https://github.com/madler/zlib/archive/refs/tags/v${PROJECT_VERSIONS[ZLIB]}.tar.gz;zlib-${PROJECT_VERSIONS[ZLIB]}.tar.gz;3rd_party/mingw64"
export PROJECT_DOWNLOADS
