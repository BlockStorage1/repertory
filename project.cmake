set(CMAKE_CXX_FLAGS "-include common.hpp ${CMAKE_CXX_FLAGS}")

if (PROJECT_IS_DARWIN)
  configure_file(
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Info.plist.in
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Info.plist
    @ONLY
  )
endif()

add_project_library(lib${PROJECT_NAME} "" "" "${PROJECT_ADDITIONAL_SOURCES}")

add_project_executable(${PROJECT_NAME} lib${PROJECT_NAME} lib${PROJECT_NAME})
if (PROJECT_IS_DARWIN)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/Info.plist
  )
endif()

add_project_test_executable(${PROJECT_NAME}_test lib${PROJECT_NAME} lib${PROJECT_NAME})
