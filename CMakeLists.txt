cmake_minimum_required(VERSION 3.22)
project(repertory)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

include(CheckIncludeFileCXX)
include(CheckIncludeFiles)
include(ExternalProject)

set(REPERTORY_MAJOR 2)
set(REPERTORY_MINOR 0)
set(REPERTORY_REV 0)
set(REPERTORY_RELEASE_NUM 0)
set(REPERTORY_RELEASE_ITER rc)
set(REPERTORY_VERSION ${REPERTORY_MAJOR}.${REPERTORY_MINOR}.${REPERTORY_REV}-${REPERTORY_RELEASE_ITER})

include(cmake/arch_detection.cmake)
include(cmake/os_detection.cmake)
include(cmake/options.cmake)
include(cmake/settings.cmake)
include(cmake/flags.cmake)
include(cmake/versions.cmake)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/cacert.pem")
  file(REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/cacert.pem)
endif()
file(DOWNLOAD https://curl.haxx.se/ca/cacert.pem ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/cacert.pem)

unset(REPERTORY_GIT_REV CACHE)
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_VARIABLE REPERTORY_GIT_REV
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (UNIX)
  check_include_files(sys/xattr.h HAS_SETXATTR)
  if (HAS_SETXATTR)
    set(REPERTORY_DEFINITIONS ${REPERTORY_DEFINITIONS} -DHAS_SETXATTR)
  endif()
endif()

if (MSVC OR MINGW)
  set(WINFSP_LIBRARY_BASENAME winfsp-x64)

  include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/3rd_party/winfsp-${WINFSP_VERSION}/inc)
  link_directories(${CMAKE_SOURCE_DIR}/3rd_party/winfsp-${WINFSP_VERSION}/lib)
  set(REPERTORY_LINK_LIBRARIES
    ${REPERTORY_LINK_LIBRARIES}
    ${WINFSP_LIBRARY_BASENAME}.lib
  )
endif()

if (LINUX OR MINGW)
  if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
    message(FATAL_ERROR "Require at least gcc-8.0")
  endif()

  set(ENV{PKG_CONFIG_PATH}
    "${EXTERNAL_BUILD_ROOT}/lib/pkgconfig:${EXTERNAL_BUILD_ROOT}/lib64/pkgconfig:$ENV{PKG_CONFIG_PATH}"
  )

  find_package(PkgConfig REQUIRED)

  set(Boost_USE_STATIC_LIBS ON)

  if (NOT MINGW)
    include_directories(/usr/local/include)
    link_directories(
      /usr/local/lib
      /usr/local/lib64
    )
  endif()

  if (REPERTORY_MUSL)
    find_library(LIBFUSE3 NO_CACHE NAMES libfuse3.a)
  else()
    if (NOT MINGW)
      find_library(LIBDL_LIBRARY NO_CACHE NAMES dl)
      find_library(LIBFUSE3 NO_CACHE NAMES fuse3)
    endif()
  endif()

  if (REPERTORY_MUSL OR MINGW)
    find_library(LIBPTHREAD_LIBRARY NO_CACHE NAMES libpthread.a)
  else()
    find_library(LIBPTHREAD_LIBRARY NO_CACHE NAMES pthread)
  endif()

  if (LIBFUSE3 AND NOT MINGW)
    pkg_check_modules(LIBFUSE3 REQUIRED fuse3>=3.0.0)
    set(LIBFUSE3_LIBRARIES ${LIBFUSE3})
    add_definitions(-DFUSE_USE_VERSION=30)
  endif()

  if(NOT LIBFUSE3_LIBRARIES AND NOT MINGW)
    pkg_check_modules(LIBFUSE2 REQUIRED fuse>=2.9.0)
    add_definitions(-DFUSE_USE_VERSION=29)
  endif()

  if (NOT LIBPTHREAD_LIBRARY) 
    message(FATAL_ERROR "'libpthread' not found")
  endif()

  if (REPERTORY_MUSL)
    if (LIBFUSE3_LIBRARIES)
      set(LIBFUSE3_LIBRARIES libfuse3.a)
    else()
      set(LIBFUSE2_LIBRARIES libfuse.a)
    endif()
  
    set(REPERTORY_LINK_LIBRARIES
      ${REPERTORY_LINK_LIBRARIES}
      ${LIBPTHREAD_LIBRARY} 
    )
  else()
    if (NOT MINGW AND NOT LIBDL_LIBRARY) 
      message(FATAL_ERROR "'libdl' not found")
    endif()

    set(REPERTORY_LINK_LIBRARIES
      ${REPERTORY_LINK_LIBRARIES}
      ${LIBPTHREAD_LIBRARY} 
      ${LIBDL_LIBRARY}
    )
  endif()
elseif (FREEBSD)
  message(FATAL_ERROR "FreeBSD is currently not supported")
elseif (MACOS)
  set(CMAKE_OSX_ARCHITECTURES x86_64)

  find_library(LIBDL_LIBRARY NO_CACHE NAMES dl)
  if (NOT LIBDL_LIBRARY) 
    message(FATAL_ERROR "'libdl' not found")
  endif()

  find_library(LIBPTHREAD_LIBRARY NO_CACHE NAMES pthread)
  if (NOT LIBPTHREAD_LIBRARY) 
    message(FATAL_ERROR "'libpthread' not found")
  endif()

  set(REPERTORY_DEFINITIONS ${REPERTORY_DEFINITIONS} -DBOOST_ASIO_HAS_STD_STRING_VIEW)
  add_definitions(-DFUSE_USE_VERSION=29)

  include_directories(/usr/local/include)
  find_library(OSXFUSE NO_CACHE NAMES OSXFUSE)
  if (NOT OSXFUSE)
    message(FATAL_ERROR "FUSE for macOS not found (https://osxfuse.github.io/)")
  endif ()
  set(LIBFUSE2_LIBRARIES fuse)
  set(REPERTORY_LINK_LIBRARIES
    ${REPERTORY_LINK_LIBRARIES}
    ${LIBDL_LIBRARY}
    ${LIBPTHREAD_LIBRARY}
    "-L/usr/local/lib"
    "-framework Foundation"
    "-framework AppKit"
    "-framework Security"
    "-framework SystemConfiguration"
  )
elseif (MSVC)
  set(OPENSSL_USE_STATIC_LIBS TRUE)
  if (NOT OPENSSL_ROOT_DIR)
    set (OPENSSL_ROOT_DIR "$ENV{USERPROFILE}\\scoop\\apps\\openssl\\current")
  endif()
  find_package(OpenSSL 1.1.1 REQUIRED)
  set(OPENSSL_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
  set(REPERTORY_DEFINITIONS ${REPERTORY_DEFINITIONS} -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

  if (NOT CMAKE_GENERATOR_LOWER STREQUAL "nmake makefiles")
    set(REPERTORY_OUTPUT_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
  endif()
endif()

if (MINGW)
  set(REPERTORY_LINK_LIBRARIES
    ${REPERTORY_LINK_LIBRARIES}
    mswsock
  )
endif()

if (MSVC OR MINGW)
  set(REPERTORY_VER_FILEVERSION ${REPERTORY_MAJOR},${REPERTORY_MINOR},${REPERTORY_REV},${REPERTORY_RELEASE_NUM})
  set(REPERTORY_VER_FILEVERSION_STR ${REPERTORY_MAJOR}.${REPERTORY_MINOR}.${REPERTORY_REV}.${REPERTORY_RELEASE_NUM})
  set(REPERTORY_VER_PRODUCTVERSION ${REPERTORY_MAJOR},${REPERTORY_MINOR},${REPERTORY_REV},${REPERTORY_RELEASE_NUM})
  set(REPERTORY_VER_PRODUCTVERSION_STR ${REPERTORY_MAJOR}.${REPERTORY_MINOR}.${REPERTORY_REV}.${REPERTORY_RELEASE_NUM})
  set(REPERTORY_VER_PRERELEASE VS_FF_PRERELEASE)
  set(REPERTORY_VER_COMPANYNAME_STR "https://git.fifthgrid.com/blockstorage")
  set(REPERTORY_VER_LEGALCOPYRIGHT_STR "Copyright 2018-2023 <MIT License> <https://git.fifthgrid.com/blockstorage/repertory>")
  set(REPERTORY_VER_FILEDESCRIPTION_STR "Mount utility for Sia and S3")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in ${CMAKE_CURRENT_SOURCE_DIR}/src/version.rc @ONLY)
  set(WINDOWS_VERSION_RC ${CMAKE_CURRENT_SOURCE_DIR}/src/version.rc)

  if (MSVC)
    set(LIB_EXT .lib)
  endif()

  set(REPERTORY_LINK_LIBRARIES
    ${REPERTORY_LINK_LIBRARIES}
    advapi32${LIB_EXT}
    bcrypt${LIB_EXT}
    comdlg32${LIB_EXT}
    crypt32${LIB_EXT}
    gdi32${LIB_EXT}
    httpapi${LIB_EXT}
    kernel32${LIB_EXT}
    ncrypt${LIB_EXT}
    ole32${LIB_EXT}
    oleaut32${LIB_EXT}
    rpcrt4${LIB_EXT}
    secur32${LIB_EXT}
    shell32${LIB_EXT}
    shlwapi${LIB_EXT}
    user32${LIB_EXT}
    userenv${LIB_EXT}
    uuid${LIB_EXT}
    version${LIB_EXT}
    winhttp${LIB_EXT}
    wininet${LIB_EXT}
    winspool${LIB_EXT}
    ws2_32${LIB_EXT}
  )
endif()

include_directories(SYSTEM
  ${EXTERNAL_BUILD_ROOT}/include
)

link_directories(
  ${EXTERNAL_BUILD_ROOT}/lib
  ${EXTERNAL_BUILD_ROOT}/lib64
)

include(cmake/zlib.cmake)
include(cmake/openssl.cmake)
include(cmake/curl.cmake)
include(cmake/boost.cmake)
include(cmake/libuuid.cmake)
include(cmake/rocksdb.cmake)
include(cmake/libsodium.cmake)

include_directories(include)
include_directories(SYSTEM
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/cpp-httplib
  ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/json
  ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/pugixml/src
  ${CURL_INCLUDE_DIRS}
  ${LIBFUSE2_INCLUDE_DIRS}
  ${LIBFUSE3_INCLUDE_DIRS}
  ${LIBUUID_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
  ${ROCKSDB_INCLUDE_DIRS}
)

set(REPERTORY_LINK_LIBRARIES
  ${ROCKSDB_LIBRARIES}
  ${LIBFUSE2_LIBRARIES}
  ${LIBFUSE3_LIBRARIES}
  ${LIBUUID_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CURL_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${LIBSODIUM_LIBRARIES}
  ${REPERTORY_LINK_LIBRARIES}
)

file(GLOB_RECURSE REPERTORY_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/pugixml/src/*.hpp
)

file(GLOB_RECURSE REPERTORY_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/pugixml/src/*.cpp
)
list(REMOVE_ITEM REPERTORY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/common.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/src/common.cpp @ONLY)

include(cmake/helpers.cmake)
include(cmake/librepertory.cmake)
include(cmake/repertory.cmake)
include(cmake/unittests.cmake)
include(cmake/install.cmake)

message(STATUS "Repertory Version: ${REPERTORY_VERSION}")
message(STATUS "Git Revision: ${REPERTORY_GIT_REV}")
