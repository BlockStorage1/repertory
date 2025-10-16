if(PROJECT_ENABLE_STDUUID)
  if(PROJECT_BUILD)
    add_definitions(-DPROJECT_ENABLE_STDUUID)

    find_package(stduuid REQUIRED)

    include_directories(BEFORE SYSTEM ${stduuid_INCLUDE_DIRS})

    link_libraries(${stduuid_LIBRARIES})
  elseif(NOT PROJECT_IS_MINGW OR CMAKE_HOST_WIN32)
    ExternalProject_Add(stduuid_project
      PREFIX external
      URL ${PROJECT_3RD_PARTY_DIR}/stduuid-${STDUUID_VERSION}.tar.gz
      URL_HASH SHA256=${STDUUID_HASH}
      LIST_SEPARATOR |
      CMAKE_ARGS ${PROJECT_EXTERNAL_CMAKE_FLAGS}
        -DBUILD_SHARED_LIBS=${PROJECT_BUILD_SHARED_LIBS}
        -DBUILD_STATIC_LIBS=ON
        -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
        -DUUID_BUILD_TESTS=OFF
        -DUUID_ENABLE_INSTALL=ON
        -DUUID_USING_CXX20_SPAN=ON
    )

    list(APPEND PROJECT_DEPENDENCIES stduuid_project)
  endif()
endif()
