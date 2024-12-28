if(PROJECT_ENABLE_JSON)
  if(PROJECT_BUILD)
    add_definitions(-DPROJECT_ENABLE_JSON)

    find_package(nlohmann_json ${JSON_VERSION} REQUIRED)

    if(PROJECT_IS_MINGW AND PROJECT_IS_MINGW_UNIX)
      include_directories(BEFORE SYSTEM /mingw64/include/nlohmann)
    else()
      include_directories(BEFORE SYSTEM ${PROJECT_EXTERNAL_BUILD_ROOT}/include/nlohmann)
    endif()
  elseif(NOT PROJECT_IS_MINGW OR CMAKE_HOST_WIN32)
    ExternalProject_Add(json_project
      PREFIX external
      URL ${PROJECT_3RD_PARTY_DIR}/json-${JSON_VERSION}.tar.gz
      URL_HASH SHA256=${JSON_HASH}
      LIST_SEPARATOR |
      CMAKE_ARGS ${PROJECT_EXTERNAL_CMAKE_FLAGS}
        -DBUILD_SHARED_LIBS=${PROJECT_BUILD_SHARED_LIBS}
        -DBUILD_STATIC_LIBS=ON
        -DJSON_BuildTests=OFF
        -DJSON_Install=ON
        -DJSON_MultipleHeaders=OFF
    )

    list(APPEND PROJECT_DEPENDENCIES json_project)
  endif()
endif()
