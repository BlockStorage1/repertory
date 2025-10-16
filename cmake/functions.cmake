if (PROJECT_MACOS_ICNS_NAME)
  set(PROJECT_MACOS_ICNS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/${PROJECT_MACOS_ICNS_NAME}")
endif()

function(set_common_target_options name)
  target_compile_definitions(${name} PUBLIC 
    ${PROJECT_DEFINITIONS}
    ${${name}_DEFINITIONS}
  )

  target_include_directories(${name} BEFORE PUBLIC 
    ${PROJECT_EXTERNAL_BUILD_ROOT}/include
  )

  target_link_directories(${name} BEFORE PUBLIC
    ${PROJECT_EXTERNAL_BUILD_ROOT}/lib
  )

  if (PROJECT_STATIC_LINK)
    target_compile_definitions(${name} PRIVATE U_STATIC_IMPLEMENTATION)
  endif()

  target_link_libraries(${name} PRIVATE 
    ICU::io
    ICU::i18n
    ICU::uc
    ICU::data
  )

  target_include_directories(${name} AFTER PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include
    ${name}_INCLUDES
  )

  if(PROJECT_DEPENDENCIES)
    add_dependencies(${name} ${PROJECT_DEPENDENCIES})
  endif()

  if(PROJECT_STATIC_LINK)
    set_property(TARGET ${name} PROPERTY LINK_SEARCH_START_STATIC 1)
  endif()
endfunction(set_common_target_options)

function(add_project_executable2 name dependencies libraries headers sources is_win32)
  if (PROJECT_WINDOWS_VERSION_RC)
    list(APPEND sources ${PROJECT_WINDOWS_VERSION_RC})
  endif()

  if (PROJECT_IS_DARWIN AND PROJECT_MACOS_ICNS_SOURCE AND "${name}" STREQUAL "${PROJECT_NAME}")
    set_source_files_properties(${PROJECT_MACOS_ICNS_SOURCE} PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
    )

    add_executable(${name}
      MACOSX_BUNDLE
      ${headers}
      ${sources}
      ${PROJECT_MACOS_ICNS_SOURCE}
      ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/main.cpp
    )

    set_target_properties(${name} PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_ICON_FILE "${PROJECT_MACOS_ICNS_NAME}"
      RESOURCE "${PROJECT_MACOS_ICNS_SOURCE}"
    )
  else()
    add_executable(${name}
      ${headers}
      ${sources}
      ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/main.cpp
    )
  endif()

  foreach(dependency ${dependencies})
    set_common_target_options(${dependency})
  endforeach()

  set_common_target_options(${name})

  if(dependencies)
    add_dependencies(${name} ${dependencies})
  endif()

  target_link_libraries(${name} PRIVATE ${libraries})

  if(PROJECT_ENABLE_SDL AND PROJECT_IS_MINGW)
    target_link_libraries(${name} PRIVATE SDL2::SDL2main)
  endif ()

  if (is_win32 AND PROJECT_IS_MINGW)
    target_link_options(${name} PRIVATE -mwindows)
  endif()
endfunction(add_project_executable2)

function(add_project_executable name dependencies libraries)
  file(GLOB_RECURSE headers
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/*.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/*.hxx
  )

  file(GLOB_RECURSE sources
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.cxx
  )

  if(PROJECT_ENABLE_WXWIDGETS OR PROJECT_ENABLE_SDL OR PROJECT_ENABLE_SFML OR PROJECT_ENABLE_NANA)
    set(IS_WIN32 ON)
  endif()

  add_project_executable2(${name} "${dependencies}" "${libraries}" "${headers}" "${sources}" "${IS_WIN32}")

  if(PROJECT_ENABLE_WXWIDGETS)
    target_link_libraries(${name} PRIVATE ${wxWidgets_LIBRARIES})
  endif()
endfunction(add_project_executable)

function(add_project_library name dependencies libraries additional_sources)
  file(GLOB_RECURSE headers
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/*.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/${name}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/${name}/include/${name}/include/*.hxx
  )

  file(GLOB_RECURSE sources
    ${additional_sources}
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.cxx
  )

  add_library(${name} STATIC 
    ${headers}
    ${sources}
  )

  set_common_target_options(${name})

  set_target_properties(${name} PROPERTIES PREFIX "")
  target_link_libraries(${name} PRIVATE ${libraries}) 
endfunction(add_project_library)

function(add_project_test_executable name dependencies libraries)
  if(PROJECT_ENABLE_TESTING)
    find_package(GTest ${GTEST_VERSION} REQUIRED)
    enable_testing()

    file(GLOB_RECURSE headers
      ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/${name}/include/${name}/include/*.hxx
      ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/${name}/include/*.hpp
      ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/*.h
      ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/include/*.hh
      ${PROJECT_SUPPORT_DIR}/test/include/*.hpp
    )

    file(GLOB_RECURSE sources
      ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.c
      ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${name}/src/*.cxx
      ${PROJECT_SUPPORT_DIR}/test/src/*.cpp
      ${additional_sources}
    )

    add_project_executable2(${name} "${dependencies}" "${libraries}" "${headers}" "${sources}" OFF)

    target_compile_definitions(${name} PRIVATE -DPROJECT_TESTING)

    target_include_directories(${name} SYSTEM BEFORE
      ${GTEST_INCLUDE_DIRS}
    )

    target_include_directories(${name} AFTER PRIVATE
      ${PROJECT_SUPPORT_DIR}/test/include
    )

    target_link_libraries(${name} PRIVATE
      GTest::gtest 
      GTest::gmock 
    )
  endif()
endfunction()
