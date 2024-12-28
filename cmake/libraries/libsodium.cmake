if(PROJECT_ENABLE_LIBSODIUM)
  if(PROJECT_BUILD)
    pkg_check_modules(SODIUM libsodium>=${LIBSODIUM_VERSION} REQUIRED)

    add_definitions(-DPROJECT_ENABLE_LIBSODIUM)

    include_directories(BEFORE SYSTEM ${SODIUM_INCLUDE_DIRS})

    if(PROJECT_IS_MINGW_UNIX)
      if(PROJECT_STATIC_LINK)
        link_libraries(/mingw64/lib/libsodium.a)
      else()
        link_libraries(/mingw64/lib/libsodium.dll.a)
      endif()
    else()
      link_libraries(${SODIUM_LIBRARIES})
    endif()
  elseif(NOT PROJECT_IS_MINGW)
    if(PROJECT_IS_MINGW)
      set(LIBSODIUM_TYPE mingw64)
    else()
      set(LIBSODIUM_TYPE linux)
    endif()

    if(PROJECT_BUILD_SHARED_LIBS)
      set(LIBSODIUM_ENABLE_SHARED yes)
    else()
      set(LIBSODIUM_ENABLE_SHARED no)
    endif()

    ExternalProject_Add(libsodium_project
      PREFIX external
      URL ${PROJECT_3RD_PARTY_DIR}/libsodium-${LIBSODIUM_VERSION}.tar.gz
      URL_HASH SHA256=${LIBSODIUM_HASH}
      BUILD_IN_SOURCE 1
      LIST_SEPARATOR |
      CONFIGURE_COMMAND ${PROJECT_3RD_PARTY_DIR}/libsodium_configure.sh 
        ${LIBSODIUM_TYPE}
        ${PROJECT_MARCH}
        ${PROJECT_EXTERNAL_BUILD_ROOT}
        ${LIBSODIUM_ENABLE_SHARED}
      BUILD_COMMAND make -j1
      INSTALL_COMMAND make install
    )

    list(APPEND PROJECT_DEPENDENCIES libsodium_project)
  endif()
endif()
