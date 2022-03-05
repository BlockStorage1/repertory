cmake_minimum_required(VERSION 3.22)
project(repertory)

include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(ExternalProject)

set(AWSCPP_VERSION 1.9.206)
set(BOOST_MAJOR_VERSION 1)
set(BOOST_MINOR_VERSION 78)
set(BOOST_MSVC_TOOLSET msvc-14.2)
set(BOOST_MSVC_TOOLSET_DLL 142) 
set(BOOST_REVISION 0)
set(BOOST_VERSION ${BOOST_MAJOR_VERSION}.${BOOST_MINOR_VERSION}.${BOOST_REVISION})
set(BOOST_VERSION_DL ${BOOST_MAJOR_VERSION}_${BOOST_MINOR_VERSION}_${BOOST_REVISION})
set(BOOST_VERSION_DLL ${BOOST_MAJOR_VERSION}_${BOOST_MINOR_VERSION})
set(CRYPTOPP_VERSION 8_2_0)
set(CURL_VERSION 7_81_0)
set(GTEST_VERSION 1.11.0)
set(LIBHTTPSERVER_VERSION 0.18.2)
set(LIBMICROHTTPD_VERSION 0.9.73)
set(LIBUUID_VERSION 1.6.2)
set(OPENSSL_VERSION 1_1_1m)
set(ROCKSDB_VERSION 6.27.3)
set(WINFSP_VERSION 1.10)
set(ZLIB_VERSION v1.2.11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(REPERTORY_ENABLE_TESTING "Enable testing" ON)

option(REPERTORY_ENABLE_S3 "Enable S3 mount support" OFF)
if (REPERTORY_ENABLE_S3)
  add_definitions(-DREPERTORY_ENABLE_S3)
  option(REPERTORY_ENABLE_S3_TESTING "Enable S3 unit tests" OFF)
  if (REPERTORY_ENABLE_S3_TESTING)
    add_definitions(-DREPERTORY_ENABLE_S3_TESTING)
  endif ()
endif ()

option(REPERTORY_ENABLE_SKYNET "Enable Skynet mount support" ON)
if (REPERTORY_ENABLE_SKYNET)
  add_definitions(-DREPERTORY_ENABLE_SKYNET)
  option(REPERTORY_ENABLE_SKYNET_PREMIUM_TESTS "Enable Skynet premium portal unit tests" OFF)
  if (REPERTORY_ENABLE_SKYNET_PREMIUM_TESTS)
    add_definitions(-DREPERTORY_ENABLE_SKYNET_PREMIUM_TESTS)
  endif()
endif ()

set(REPERTORY_MAJOR 2)
set(REPERTORY_MINOR 0)
set(REPERTORY_REV 0)
set(REPERTORY_RELEASE_NUM 2000000)
set(REPERTORY_RELEASE_ITER rc)
set(REPERTORY_VERSION ${REPERTORY_MAJOR}.${REPERTORY_MINOR}.${REPERTORY_REV}-${REPERTORY_RELEASE_ITER})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(IS_CLANG_COMPILER TRUE)
endif()

if(UNIX AND NOT APPLE)
  if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(FREEBSD TRUE)
  else()
    set(LINUX TRUE)
  endif()
elseif(UNIX AND APPLE)
  set(MACOS TRUE)
  set(CMAKE_OSX_ARCHITECTURES x86_64)
endif()

if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
  message(FATAL_ERROR "32-bit compilation is not supported")
else()
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*|ARM64.*)")
    set(REPERTORY_UNIX_ARCH armv8-a)
    set(IS_ARM64 TRUE)
    add_definitions("-DIS_ARM64")
  else()
    set(REPERTORY_UNIX_ARCH x86-64)
  endif()

  set(WINFSP_LIBRARY_BASENAME winfsp-x64)
  set(REPERTORY_LIB_DIR lib64)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/cacert.pem")
  file(REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/cacert.pem)
endif()
file(DOWNLOAD https://curl.haxx.se/ca/cacert.pem ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/cacert.pem)

execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_VARIABLE REPERTORY_GIT_REV
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG_EXTRA d)
  set(DEBUG_EXTRA2 -d)
  set(DEBUG_EXTRA3 _d)
  set(BOOST_DEBUG_EXTRA gd-)
  set(CMAKE_BUILD_TYPE_LOWER debug)
else()
  set(CMAKE_BUILD_TYPE_LOWER release)
endif()
string(TOLOWER "${CMAKE_GENERATOR}" CMAKE_GENERATOR_LOWER)

set(EXTERNAL_BUILD_ROOT ${CMAKE_BINARY_DIR}/external)
set(EXTERNAL_BUILD_TYPE ${CMAKE_BUILD_TYPE})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/common.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/src/common.cpp @ONLY)

link_directories(
  ${EXTERNAL_BUILD_ROOT}/lib
)

if (UNIX)
  set(STATIC_LIB_EXT a)

  set(REPERTORY_C_FLAGS_LIST
    ${REPERTORY_C_FLAGS_LIST}
    -march=${REPERTORY_UNIX_ARCH}
    -mtune=generic
    -D_FILE_OFFSET_BITS=64
  )

  set(REPERTORY_CXX_FLAGS_LIST
    ${REPERTORY_CXX_FLAGS_LIST}
    -march=${REPERTORY_UNIX_ARCH}
    -mtune=generic
    -D_FILE_OFFSET_BITS=64
    -std=c++${CMAKE_CXX_STANDARD}
  )

  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(REPERTORY_C_FLAGS_LIST
      ${REPERTORY_C_FLAGS_LIST}
      -g
      -gdwarf-4
      -DDEBUG
      -O0
      -fno-omit-frame-pointer
      -D_DEBUG
      -Wall
      -Wextra
      -Wpedantic
    )

    set(REPERTORY_CXX_FLAGS_LIST
      ${REPERTORY_CXX_FLAGS_LIST}
      -g
      -gdwarf-4
      -DDEBUG
      -O0
      -fno-omit-frame-pointer
      -D_DEBUG
      -Wall
      -Wextra
      -Wpedantic
    )
  else ()
    set(REPERTORY_C_FLAGS_LIST
      ${REPERTORY_C_FLAGS_LIST}
      -O3
      -DNDEBUG
    )

    set(REPERTORY_CXX_FLAGS_LIST
      ${REPERTORY_CXX_FLAGS_LIST}
      -O3
      -DNDEBUG
    )

    if (NOT IS_CLANG_COMPILER)
      set(REPERTORY_C_FLAGS_LIST
        ${REPERTORY_C_FLAGS_LIST}
        -s
      )

      set(REPERTORY_CXX_FLAGS_LIST
        ${REPERTORY_CXX_FLAGS_LIST}
        -s
      )
    endif ()
  endif()
endif()

if (LINUX)
  execute_process(COMMAND /bin/bash ${CMAKE_CURRENT_SOURCE_DIR}/detect_linux_build.sh OUTPUT_VARIABLE LINUX_DISTRO)
  string(REGEX REPLACE "\n$" "" LINUX_DISTRO "${LINUX_DISTRO}")
  if (LINUX_DISTRO STREQUAL "debian9")
    set(IS_DEBIAN9_DISTRO TRUE)
    add_definitions("-DIS_DEBIAN9_DISTRO")
  endif()

  set(ENV{PKG_CONFIG_PATH} "${EXTERNAL_BUILD_ROOT}/lib/pkgconfig:${EXTERNAL_BUILD_ROOT}/${REPERTORY_LIB_DIR}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBFUSE REQUIRED fuse>=2.9.0)

  set(Boost_USE_STATIC_LIBS ON)

  include_directories(/usr/local/include)

  if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.5)
    message(FATAL_ERROR "Require at least gcc-5.5")
  endif()

  link_directories(
    ${EXTERNAL_BUILD_ROOT}/${REPERTORY_LIB_DIR}
    /usr/local/lib
    /usr/local/${REPERTORY_LIB_DIR}
  )
  set(REPERTORY_LINK_LIBRARIES
    dl
    pthread
  )
elseif (FREEBSD)
  message(FATAL_ERROR "FreeBSD is no longer supported")
elseif (MACOS)
  add_definitions("-DBOOST_ASIO_HAS_STD_STRING_VIEW")
  include_directories(/usr/local/include)
  find_library(OSXFUSE NAMES OSXFUSE)
  if (OSXFUSE-NOTFOUND)
    message(FATAL_ERROR "FUSE for macOS not found (https://osxfuse.github.io/)")
  endif ()
  set(LIBFUSE_LIBRARIES fuse)
  set(REPERTORY_LINK_LIBRARIES
    z
    pthread
    dl
    "-L/usr/local/lib"
    "-framework Foundation"
    "-framework AppKit"
    "-framework SystemConfiguration"
  )
elseif (MSVC)
  set(STATIC_LIB_EXT lib)

  set(REPERTORY_C_FLAGS_LIST
    ${REPERTORY_C_FLAGS_LIST}
    /D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
    /DNOMINMAX
    /bigobj
    /Zi
  )

  set(REPERTORY_CXX_FLAGS_LIST
    ${REPERTORY_CXX_FLAGS_LIST}
    /D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
    /DNOMINMAX
    /bigobj
    /Zi
  )

  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(REPERTORY_C_FLAGS_LIST
      ${REPERTORY_C_FLAGS_LIST}
      /DEBUG
    )

    set(REPERTORY_CXX_FLAGS_LIST
      ${REPERTORY_CXX_FLAGS_LIST}
      /DEBUG
    )

    set(REPERTORY_SHARED_LINKER_FLAGS_LIST
      ${REPERTORY_SHARED_LINKER_FLAGS_LIST}
      /DEBUG
      /OPT:REF
      /OPT:ICF
    )
  endif()

  set(REPERTORY_VER_FILEVERSION ${REPERTORY_MAJOR},${REPERTORY_MINOR},${REPERTORY_REV},${REPERTORY_RELEASE_NUM})
  set(REPERTORY_VER_FILEVERSION_STR ${REPERTORY_MAJOR}.${REPERTORY_MINOR}.${REPERTORY_REV}.${REPERTORY_RELEASE_NUM})
  set(REPERTORY_VER_PRODUCTVERSION ${REPERTORY_MAJOR},${REPERTORY_MINOR},${REPERTORY_REV},${REPERTORY_RELEASE_NUM})
  set(REPERTORY_VER_PRODUCTVERSION_STR ${REPERTORY_MAJOR}.${REPERTORY_MINOR}.${REPERTORY_REV}.${REPERTORY_RELEASE_NUM})
  set(REPERTORY_VER_PRERELEASE VS_FF_PRERELEASE)
  set(REPERTORY_VER_COMPANYNAME_STR "https://git.fifthgrid.com/blockstorage")
  set(REPERTORY_VER_LEGALCOPYRIGHT_STR "Copyright 2018 <MIT License> <https://git.fifthgrid.com/blockstorage/repertory>")
  set(REPERTORY_VER_FILEDESCRIPTION_STR "Mount utility for Sia, Skynet and S3")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in ${CMAKE_CURRENT_SOURCE_DIR}/src/version.rc @ONLY)
  set(WINDOWS_VERSION_RC ${CMAKE_CURRENT_SOURCE_DIR}/src/version.rc)

  set(OPENSSL_USE_STATIC_LIBS TRUE)
  if (NOT OPENSSL_ROOT_DIR)
    set (OPENSSL_ROOT_DIR "C:\\Program Files\\OpenSSL-Win64")
  endif()
  find_package(OpenSSL 1.1.1 REQUIRED)
  set(OPENSSL_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)

  include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/3rd_party/winfsp-${WINFSP_VERSION}/inc)
  add_definitions(-DTTMATH_NOASM -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
  link_directories(${CMAKE_SOURCE_DIR}/3rd_party/winfsp-${WINFSP_VERSION}/lib)
  set(REPERTORY_LINK_LIBRARIES
    Ncrypt.lib
    Rpcrt4.lib
    Secur32.lib
    Shlwapi.lib
    Userenv.lib
    Version.lib
    Winhttp.lib
    Wininet.lib
    Ws2_32.lib
    advapi32.lib
    bcrypt.lib
    comdlg32.lib
    crypt32.lib
    gdi32.lib
    httpapi.lib
    kernel32.lib
    ole32.lib
    oleaut32.lib
    shell32.lib
    user32.lib
    uuid.lib
    winspool.lib
    ${WINFSP_LIBRARY_BASENAME}.lib
  )

  if (CMAKE_GENERATOR MATCHES "NMake Makefiles")
    set(REPERTORY_DLL_OUTPUT_DIR ${CMAKE_BINARY_DIR})
  else()
    set(REPERTORY_DLL_OUTPUT_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
  endif()

  function(CopySupportFiles target)
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/3rd_party/winfsp-${WINFSP_VERSION}/bin/${WINFSP_LIBRARY_BASENAME}.dll
        ${REPERTORY_DLL_OUTPUT_DIR}/${WINFSP_LIBRARY_BASENAME}.dll)
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/3rd_party/cacert.pem
        ${REPERTORY_DLL_OUTPUT_DIR}/cacert.pem)
  endfunction()
endif()

list(JOIN REPERTORY_CXX_FLAGS_LIST " " REPERTORY_CXX_FLAGS_LIST)
list(JOIN REPERTORY_C_FLAGS_LIST " " REPERTORY_C_FLAGS_LIST)
list(JOIN REPERTORY_SHARED_LINKER_FLAGS_LIST " " REPERTORY_SHARED_LINKER_FLAGS_LIST)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${REPERTORY_C_FLAGS_LIST}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${REPERTORY_CXX_FLAGS_LIST}")
set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${REPERTORY_SHARED_LINKER_FLAGS_LIST}")

include(cmake/zlib.cmake)
include(cmake/openssl.cmake)
include(cmake/curl.cmake)
include(cmake/boost.cmake)
include(cmake/libuuid.cmake)
include(cmake/rocksdb.cmake)
include(cmake/cryptopp.cmake)
include(cmake/awscpp.cmake)
include(cmake/libmicrohttpd.cmake)
include(cmake/libhttpserver.cmake)

include_directories(include)
include_directories(SYSTEM
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/json
  ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/jsonrpcpp-1.1.1/lib
  ${LIBUUID_INCLUDE_DIR}
  ${EXTERNAL_BUILD_ROOT}/include
  ${EXTERNAL_BUILD_ROOT}/include/cryptopp
  ${OPENSSL_INCLUDE_DIR}
  ${CURL_INCLUDE_DIRS}
  ${LIBFUSE_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/3rd_party/ttmath-0.9.3/ttmath
  ${ROCKSDB_INCLUDE_DIRS}
)

if (NOT IS_DEBIAN9_DISTRO)
  check_include_file_cxx(optional HAS_STD_OPTIONAL)
  if (HAS_STD_OPTIONAL)
    add_definitions(-DHAS_STD_OPTIONAL)
  endif()
endif()

if (UNIX)
  macro(install_symlink filepath sympath)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${filepath} ${sympath})")
    install(CODE "message(\"-- Created symlink: ${sympath} -> ${filepath}\")")
  endmacro(install_symlink)
  check_include_files(sys/xattr.h HAS_SETXATTR)
  if (HAS_SETXATTR)
    add_definitions(-DHAS_SETXATTR)
  endif()
endif()

set(REPERTORY_LINK_LIBRARIES
  ${ROCKSDB_LIBRARIES}
  ${LIBFUSE_LIBRARIES}
  ${LIBUUID_LIBRARIES}
  ${AWSCPP_LIBRARIES}
  ${LIBHTTPSERVER_LIBRARIES}
  ${LIBMICROHTTPD_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CURL_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${REPERTORY_LINK_LIBRARIES}
)

file(GLOB_RECURSE REPERTORY_HEADERS
  ${REPERTORY_HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hh
  ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hpp
)

file(GLOB_RECURSE REPERTORY_SOURCES
  ${REPERTORY_SOURCES}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/jsonrpcpp-1.1.1/lib/jsonrp.cpp
)
list(REMOVE_ITEM REPERTORY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_library(librepertory STATIC ${REPERTORY_SOURCES} ${REPERTORY_HEADERS})
set_target_properties(librepertory PROPERTIES PREFIX "")
target_link_libraries(librepertory LINK_INTERFACE_LIBRARIES ${REPERTORY_LINK_LIBRARIES}) 

add_dependencies(librepertory
  rocksdb_project
  cryptopp_project
  libhttpserver_project
)
if (REPERTORY_ENABLE_S3)
  add_dependencies(librepertory awscpp_project)
endif ()

if (LINUX)
  add_dependencies(librepertory
    libuuid_project
    boost_project
    curl_project
    openssl_project
  )
elseif (MACOS)
  add_dependencies(librepertory
    openssl_project
    boost_project
    curl_project
  )
elseif (MSVC)
  add_dependencies(librepertory
    curl_project
    zlib_project
    boost_project
  )
endif()

add_executable(repertory
  src/main.cpp
  ${WINDOWS_VERSION_RC}
  ${REPERTORY_HEADERS}
)

add_custom_command(
  TARGET repertory
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/cacert.pem
    ${CMAKE_BINARY_DIR}/cacert.pem
)

add_dependencies(repertory librepertory)

target_link_libraries(repertory PRIVATE librepertory)

if (MACOS)
  set_target_properties(repertory PROPERTIES COMPILE_FLAGS -fvisibility=hidden)
elseif (MSVC)
  CopySupportFiles(repertory)
endif()

include(cmake/testing.cmake)

if (UNIX)
  install(TARGETS repertory RUNTIME DESTINATION bin)
  if (LINUX)
    install_symlink(${CMAKE_INSTALL_PREFIX}/bin/repertory /sbin/mount.repertory)
  endif()
endif()

message(STATUS "Repertory Version: ${REPERTORY_VERSION}")
message(STATUS "Git Revision: ${REPERTORY_GIT_REV}")
