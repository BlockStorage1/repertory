if((PROJECT_IS_DARWIN OR PROJECT_REQUIRE_ALPINE) AND NOT PROJECT_IS_MINGW AND NOT PROJECT_BUILD)
  if(PROJECT_BUILD_SHARED_LIBS)
    set(ICU_ENABLE_SHARED yes)
  else()
    set(ICU_ENABLE_SHARED no)
  endif()

  ExternalProject_Add(icu_project
    PREFIX external
    URL ${PROJECT_3RD_PARTY_DIR}/mingw64/icu-release-${ICU_VERSION}.tar.gz
    URL_HASH SHA256=${ICU_HASH}
    BUILD_IN_SOURCE 1
    LIST_SEPARATOR |
    PATCH_COMMAND chmod +x ${PROJECT_3RD_PARTY_DIR}/icu_configure.sh
    CONFIGURE_COMMAND cd icu4c/source && ${PROJECT_3RD_PARTY_DIR}/icu_configure.sh
      ${PROJECT_MARCH}
      ${PROJECT_EXTERNAL_BUILD_ROOT}
      ${ICU_ENABLE_SHARED}
    BUILD_COMMAND cd icu4c/source && make -j$ENV{CMAKE_BUILD_PARALLEL_LEVEL}
    INSTALL_COMMAND cd icu4c/source && make install
  )

  list(APPEND PROJECT_DEPENDENCIES icu_project)
endif()
