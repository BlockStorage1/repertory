set(BOOST_ADDRESS_MODEL 64)
set(BOOST_DLL_ARCH x64)
set(BOOST_DOWNLOAD_URL https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_DL}.tar.gz)

if (UNIX)
  set(BOOST_PROJECT_NAME boost_${BOOST_VERSION})
  set(BOOST_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${BOOST_PROJECT_NAME})
  if (IS_CLANG_COMPILER)
    set(BOOST_TOOLSET --with-toolset=clang)
  else ()
    set(BOOST_OPENSSL_DIR "--openssldir=${EXTERNAL_BUILD_ROOT}")
  endif()
  if (IS_ARM64)
    set (BOOST_ARCH arm)
  else()
    set (BOOST_ARCH x86)
  endif()
  set (BOOST_COMMON_ARGS 
      ${BOOST_OPENSSL_DIR}
      --prefix=${EXTERNAL_BUILD_ROOT}
      address-model=${BOOST_ADDRESS_MODEL}
      architecture=${BOOST_ARCH}
      cxxflags=-std=c++${CMAKE_CXX_STANDARD}
      cxxstd=${CMAKE_CXX_STANDARD}
      define=BOOST_ASIO_HAS_STD_STRING_VIEW
      define=BOOST_SYSTEM_NO_DEPRECATED
      link=static
      linkflags=-std=c++${CMAKE_CXX_STANDARD}
      threading=multi
  )

  ExternalProject_Add(boost_project
    DOWNLOAD_NO_PROGRESS 1
    URL ${BOOST_DOWNLOAD_URL}
    PREFIX ${BOOST_BUILD_ROOT}
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND
      ./bootstrap.sh
      ${BOOST_TOOLSET}
      --with-libraries=atomic,chrono,date_time,filesystem,random,regex,serialization,system,thread
    BUILD_COMMAND
      ./b2
      ${BOOST_COMMON_ARGS}
    INSTALL_COMMAND
      ./b2
      install
      ${BOOST_COMMON_ARGS}
  )

  add_dependencies(boost_project openssl_project)

  set(BOOST_ROOT ${BOOST_BUILD_ROOT}/src/boost_project)

  set(Boost_LIBRARIES
    libboost_system.a
    libboost_atomic.a
    libboost_chrono.a
    libboost_date_time.a
    libboost_filesystem.a
    libboost_random.a
    libboost_regex.a
    libboost_serialization.a
    libboost_thread.a
  )
elseif(MSVC)
  set(BOOST_PROJECT_NAME boost_${BOOST_VERSION})
  set(BOOST_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${BOOST_PROJECT_NAME})
  set (BOOST_COMMON_ARGS 
      --with-date_time
      --with-regex
      --with-serialization
      --with-system
      --with-filesystem
      --prefix=${EXTERNAL_BUILD_ROOT}
      runtime-link=shared
      threading=multi
      address-model=${BOOST_ADDRESS_MODEL}
      architecture=x86
      toolset=${BOOST_MSVC_TOOLSET}
      variant=${CMAKE_BUILD_TYPE_LOWER}
      -sZLIB_BINARY=zlibstatic${DEBUG_EXTRA}
      -sZLIB_LIBPATH="${EXTERNAL_BUILD_ROOT}/lib"
      -sZLIB_INCLUDE="${EXTERNAL_BUILD_ROOT}/include"
  )

  ExternalProject_Add(boost_project
    DOWNLOAD_NO_PROGRESS 1
    URL ${BOOST_DOWNLOAD_URL}
    PREFIX ${BOOST_BUILD_ROOT}
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND
      bootstrap.bat
      --with-date_time
      --with-regex
      --with-system
      --with-serialization
      --with-filesystem
    BUILD_COMMAND
      b2.exe
      ${BOOST_COMMON_ARGS}
    INSTALL_COMMAND
      b2.exe
      install
      ${BOOST_COMMON_ARGS}
  )

  add_dependencies(boost_project zlib_project)
  set(BOOST_ROOT ${BOOST_BUILD_ROOT}/src/boost_project)
  set(Boost_INCLUDE_DIR ${EXTERNAL_BUILD_ROOT}/include/boost-${BOOST_VERSION_DLL})
  set(Boost_LIBRARIES
    ${EXTERNAL_BUILD_ROOT}/lib/libboost_date_time-vc${BOOST_MSVC_TOOLSET_DLL}-mt-${BOOST_DEBUG_EXTRA}${BOOST_DLL_ARCH}-${BOOST_VERSION_DLL}.lib
    ${EXTERNAL_BUILD_ROOT}/lib/libboost_filesystem-vc${BOOST_MSVC_TOOLSET_DLL}-mt-${BOOST_DEBUG_EXTRA}${BOOST_DLL_ARCH}-${BOOST_VERSION_DLL}.lib
    ${EXTERNAL_BUILD_ROOT}/lib/libboost_regex-vc${BOOST_MSVC_TOOLSET_DLL}-mt-${BOOST_DEBUG_EXTRA}${BOOST_DLL_ARCH}-${BOOST_VERSION_DLL}.lib
    ${EXTERNAL_BUILD_ROOT}/lib/libboost_serialization-vc${BOOST_MSVC_TOOLSET_DLL}-mt-${BOOST_DEBUG_EXTRA}${BOOST_DLL_ARCH}-${BOOST_VERSION_DLL}.lib
    ${EXTERNAL_BUILD_ROOT}/lib/libboost_system-vc${BOOST_MSVC_TOOLSET_DLL}-mt-${BOOST_DEBUG_EXTRA}${BOOST_DLL_ARCH}-${BOOST_VERSION_DLL}.lib
  )
endif()
