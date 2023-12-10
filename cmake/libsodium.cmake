set(LIBSODIUM_PROJECT_NAME libsodium_${LIBSODIUM_VERSION})
set(LIBSODIUM_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${LIBSODIUM_PROJECT_NAME})
set(LIBSODIUM_BUILD_TYPE ${EXTERNAL_BUILD_TYPE})

if (MINGW)
  set(LIBSODIUM_HOST --host=x86_64-w64-mingw32)
endif()

ExternalProject_Add(libsodium_project
  DOWNLOAD_NO_PROGRESS 1
  PREFIX ${LIBSODIUM_BUILD_ROOT}
  BUILD_IN_SOURCE 1
  URL https://github.com/jedisct1/libsodium/releases/download/${LIBSODIUM_VERSION}-RELEASE/libsodium-${LIBSODIUM_VERSION}.tar.gz 
  CONFIGURE_COMMAND
    ./configure
    ${LIBSODIUM_HOST}
    --prefix=${EXTERNAL_BUILD_ROOT}
    --enable-shared=no
    --enable-static=yes
    --disable-asm
  BUILD_COMMAND make
  INSTALL_COMMAND make install
)

set(LIBSODIUM_LIBRARIES libsodium.a)

add_dependencies(libsodium_project zlib_project)
