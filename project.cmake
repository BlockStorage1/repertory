set(CMAKE_CXX_FLAGS "-include common.hpp ${CMAKE_CXX_FLAGS}")

add_project_library(lib${PROJECT_NAME} "" "" "${PROJECT_ADDITIONAL_SOURCES}")

add_project_executable(${PROJECT_NAME} lib${PROJECT_NAME} lib${PROJECT_NAME})

add_project_test_executable(${PROJECT_NAME}_test lib${PROJECT_NAME} lib${PROJECT_NAME})
