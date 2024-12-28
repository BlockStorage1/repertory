set(OPENSSL_ROOT_DIR $ENV{OPENSSL_ROOT_DIR})

if(PROJECT_ENABLE_OPENSSL)
  if(PROJECT_BUILD)
    add_definitions(-DPROJECT_ENABLE_OPENSSL)

    find_package(OpenSSL ${OPENSSL_VERSION} REQUIRED)

    include_directories(BEFORE SYSTEM ${OPENSSL_INCLUDE_DIR})

    link_libraries(
      OpenSSL::Crypto
      OpenSSL::SSL
    )
  elseif(NOT PROJECT_IS_MINGW)
    if(PROJECT_IS_MINGW)
      set(OPENSSL_COMPILE_TYPE mingw64)
    elseif(PROJECT_IS_ARM64)
      set(OPENSSL_COMPILE_TYPE linux-aarch64)
    else()
      set(OPENSSL_COMPILE_TYPE linux-x86_64)
    endif()

    if(PROJECT_IS_MINGW_UNIX)
      set(OPENSSL_CROSS_PREFIX "--cross-compile-prefix=x86_64-w64-mingw32-")
    endif()

    if(PROJECT_BUILD_SHARED_LIBS)
      set(OPENSSL_NO_SHARED shared)
    else()
      set(OPENSSL_NO_SHARED no-shared)
    endif()

    ExternalProject_Add(openssl_project
      PREFIX external
      URL ${PROJECT_3RD_PARTY_DIR}/openssl-${OPENSSL_VERSION}.tar.gz
      URL_HASH SHA256=${OPENSSL_HASH}
      BUILD_IN_SOURCE 1
      LIST_SEPARATOR |
      CONFIGURE_COMMAND ./Configure 
        ${OPENSSL_COMPILE_TYPE} 
        ${OPENSSL_CROSS_PREFIX}
        --prefix=${PROJECT_EXTERNAL_BUILD_ROOT}
        no-apps
        no-docs
        ${OPENSSL_NO_SHARED}
      BUILD_COMMAND make -j1
      INSTALL_COMMAND make install
    )

    list(APPEND PROJECT_DEPENDENCIES openssl_project)
  endif()
endif()
