if(PROJECT_ENABLE_CURL)
  if(PROJECT_BUILD)
    add_definitions(-DPROJECT_ENABLE_CURL)

    find_package(CURL ${CURL_VERSION} REQUIRED)

    include_directories(BEFORE SYSTEM ${CURL_INCLUDE_DIRS})

    link_libraries(CURL::libcurl)

    if (EXISTS ${PROJECT_DIST_DIR}/cacert.pem)
      file(REMOVE ${PROJECT_DIST_DIR}/cacert.pem)
    endif()
    file(DOWNLOAD https://curl.haxx.se/ca/cacert.pem ${PROJECT_DIST_DIR}/cacert.pem)
  elseif(NOT PROJECT_IS_MINGW OR CMAKE_HOST_WIN32)
    ExternalProject_Add(curl_project
      PREFIX external
      URL ${PROJECT_3RD_PARTY_DIR}/curl-${CURL_VERSION}.tar.gz
      URL_HASH SHA256=${CURL_HASH}
      LIST_SEPARATOR |
      BUILD_COMMAND 
        ${CMAKE_COMMAND} --build . -- -j$ENV{CMAKE_BUILD_PARALLEL_LEVEL}
      CMAKE_ARGS ${PROJECT_EXTERNAL_CMAKE_FLAGS}
        -DBUILD_CURL_EXE=OFF
        -DBUILD_LIBCURL_DOCS=OFF
        -DBUILD_MISC_DOCS=OFF
        -DBUILD_SHARED_LIBS=${PROJECT_BUILD_SHARED_LIBS}
        -DBUILD_STATIC_CURL=ON
        -DBUILD_STATIC_LIBS=ON
        -DBUILD_STATIC_LIBS=ON
        -DBUILD_TESTING=OFF
        -DCURL_BROTLI=OFF
        -DCURL_CA_BUNDLE=./cacert.pem
        -DCURL_CA_FALLBACK=ON
        -DCURL_DISABLE_LDAP=ON
        -DCURL_USE_LIBPSL=OFF
        -DCURL_USE_LIBSSH2=OFF
        -DCURL_USE_OPENSSL=${PROJECT_ENABLE_OPENSSL}
        -DCURL_ZLIB=ON
        -DENABLE_CURL_MANUAL=OFF
        -DENABLE_THREADED_RESOLVER=ON
        -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}
        -DOPENSSL_USE_STATIC_LIBS=${OPENSSL_USE_STATIC_LIBS}
        -DUSE_LIBIDN2=OFF
        -DZLIB_USE_STATIC_LIBS=${ZLIB_USE_STATIC_LIBS}
    )

    list(APPEND PROJECT_DEPENDENCIES curl_project)

    if (NOT CMAKE_HOST_WIN32)
      add_dependencies(curl_project openssl_project)
    endif()
  endif()
endif()
