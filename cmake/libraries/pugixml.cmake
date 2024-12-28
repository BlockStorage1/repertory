if(PROJECT_ENABLE_PUGIXML)
  if(PROJECT_BUILD)
    add_definitions(-DPROJECT_ENABLE_PUGIXML)

    find_package(pugixml ${PUGIXML_VERSION} REQUIRED)

    include_directories(BEFORE SYSTEM ${PUGIXML_INCLUDE_DIR})

    if(PROJECT_STATIC_LINK)
      link_libraries(pugixml::static)
    else()
      link_libraries(pugixml::pugixml)
    endif()
  elseif(NOT PROJECT_IS_MINGW)
    ExternalProject_Add(pugixml_project
      PREFIX external
      URL ${PROJECT_3RD_PARTY_DIR}/pugixml-${PUGIXML_VERSION}.tar.gz
      URL_HASH SHA256=${PUGIXML_HASH}
      LIST_SEPARATOR |
      CMAKE_ARGS ${PROJECT_EXTERNAL_CMAKE_FLAGS}
        -DBUILD_SHARED_LIBS=${PROJECT_BUILD_SHARED_LIBS}
        -DBUILD_STATIC_LIBS=ON
    )

    list(APPEND PROJECT_DEPENDENCIES pugixml_project)
  endif()
endif()
