if(PROJECT_ENABLE_SPDLOG)
  if(PROJECT_BUILD)
    add_definitions(-DPROJECT_ENABLE_SPDLOG)

    find_package(spdlog ${SPDLOG_VERSION} REQUIRED)

    include_directories(BEFORE SYSTEM ${SPDLOG_INCLUDE_DIRS})

    link_libraries(spdlog::spdlog) 
  elseif(NOT PROJECT_IS_MINGW OR CMAKE_HOST_WIN32)
    ExternalProject_Add(spdlog_project
      PREFIX external
      URL ${PROJECT_3RD_PARTY_DIR}/spdlog-${SPDLOG_VERSION}.tar.gz
      URL_HASH SHA256=${SPDLOG_HASH}
      LIST_SEPARATOR |
      CMAKE_ARGS ${PROJECT_EXTERNAL_CMAKE_FLAGS}
        -DBUILD_SHARED_LIBS=${PROJECT_BUILD_SHARED_LIBS}
        -DSPDLOG_BUILD_EXAMPLE=OFF
        -DSPDLOG_FMT_EXTERNAL=OFF
        -DSPDLOG_FMT_EXTERNAL_HO=OFF
    )

    list(APPEND PROJECT_DEPENDENCIES spdlog_project)
  endif()
endif()
