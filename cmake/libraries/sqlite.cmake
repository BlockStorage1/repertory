if(PROJECT_ENABLE_SQLITE)
  if(PROJECT_BUILD)
    add_definitions(-DPROJECT_ENABLE_SQLITE)
    if (PROJECT_IS_MINGW AND NOT PROJECT_IS_MINGW_UNIX)
      pkg_check_modules(SQLITE3 REQUIRED sqlite3>=${SQLITE2_VERSION})
      include_directories(SYSTEM BEFORE ${SQLITE3_INCLUDE_DIRS})
      link_libraries(${SQLITE3_LIBRARIES})
    else()
      set(SQLITE_SYSTEM_ROOT ${PROJECT_BUILD_DIR}/external/src/sqlite_project)

      include_directories(SYSTEM BEFORE ${SQLITE_SYSTEM_ROOT})

      list(APPEND PROJECT_ADDITIONAL_SOURCES
        ${SQLITE_SYSTEM_ROOT}/sqlite3.c
      )
    endif()
  elseif(NOT PROJECT_IS_MINGW OR PROJECT_IS_MINGW_UNIX)
    ExternalProject_Add(sqlite_project
      PREFIX external
      URL ${PROJECT_3RD_PARTY_DIR}/sqlite-amalgamation-${SQLITE_VERSION}.zip
      URL_HASH SHA256=${SQLITE_HASH}
      CONFIGURE_COMMAND echo "No configure"
      BUILD_COMMAND echo "No build"
      INSTALL_COMMAND echo "No install"
    )

    list(APPEND PROJECT_DEPENDENCIES sqlite_project)
  endif()
endif()
