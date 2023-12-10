if (IS_CLANG_COMPILER)
  set(OPENSSL_COMPILE_TYPE_EXTRA -clang)
endif()

if (MACOS)
  set(OPENSSL_COMPILE_TYPE darwin64-x86_64-cc)
elseif(IS_ARM64)
  set(OPENSSL_COMPILE_TYPE linux-aarch64${OPENSSL_COMPILE_TYPE_EXTRA})
elseif(MINGW)
  if (CMAKE_TOOLCHAIN_FILE)
    set(OPENSSL_COMPILE_TYPE --cross-compile-prefix=x86_64-w64-mingw32- mingw64${OPENSSL_COMPILE_TYPE_EXTRA})
  else()
    set(OPENSSL_COMPILE_TYPE mingw64${OPENSSL_COMPILE_TYPE_EXTRA})
  endif()
else()
  set(OPENSSL_COMPILE_TYPE linux-x86_64${OPENSSL_COMPILE_TYPE_EXTRA})
endif()

set(OPENSSL_PROJECT_NAME openssl_${OPENSSL_VERSION})
set(OPENSSL_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${OPENSSL_PROJECT_NAME})
ExternalProject_Add(openssl_project
  DOWNLOAD_NO_PROGRESS 1
  URL https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz 
  PREFIX ${OPENSSL_BUILD_ROOT}
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ./Configure no-shared ${OPENSSL_COMPILE_TYPE} --openssldir=${EXTERNAL_BUILD_ROOT}/ssl --prefix=${EXTERNAL_BUILD_ROOT}
  BUILD_COMMAND make -j1
  INSTALL_COMMAND make install
)

set(OPENSSL_LIBRARIES
  ${EXTERNAL_BUILD_ROOT}/lib/libssl.a
  ${EXTERNAL_BUILD_ROOT}/lib/libcrypto.a
)

add_dependencies(openssl_project zlib_project)
