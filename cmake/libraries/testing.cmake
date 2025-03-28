if (PROJECT_ENABLE_TESTING)
  if(PROJECT_BUILD)
    add_definitions(-DPROJECT_ENABLE_TESTING)
  elseif(NOT PROJECT_IS_MINGW)
    ExternalProject_Add(gtest_project
      PREFIX external
      URL ${PROJECT_3RD_PARTY_DIR}/googletest-${GTEST_VERSION}.tar.gz
      URL_HASH SHA256=${GTEST_HASH}
      LIST_SEPARATOR |
      CMAKE_ARGS ${PROJECT_EXTERNAL_CMAKE_FLAGS}
        -DBUILD_SHARED_LIBS=${PROJECT_BUILD_SHARED_LIBS}
        -DBUILD_STATIC_LIBS=ON
    )

    list(APPEND PROJECT_DEPENDENCIES gtest_project)
  endif()
endif()
