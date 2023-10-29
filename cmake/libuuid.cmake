if (LINUX)
  set(LIBUUID_PROJECT_NAME libuuid_${LIBUUID_VERSION})
  set(LIBUUID_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${LIBUUID_PROJECT_NAME})
  #URL "https://www.mirrorservice.org/sites/ftp.ossp.org/pkg/lib/uuid/uuid-${LIBUUID_VERSION}.tar.gz"
  ExternalProject_Add(libuuid_project
    DOWNLOAD_NO_PROGRESS 1
    PREFIX ${LIBUUID_BUILD_ROOT}
    URL https://src.fedoraproject.org/repo/pkgs/uuid/uuid-${LIBUUID_VERSION}.tar.gz/5db0d43a9022a6ebbbc25337ae28942f/uuid-${LIBUUID_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/config.guess ./config.guess && 
      cp ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/config.sub ./config.sub && 
      CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER};./configure --disable-shared --enable-static --prefix=${EXTERNAL_BUILD_ROOT}
    BUILD_COMMAND CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER};make
    INSTALL_COMMAND make install
  )
  set(LIBUUID_LIBRARIES libuuid.a)
endif()
