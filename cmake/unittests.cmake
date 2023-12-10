# Testing
if (REPERTORY_ENABLE_TESTING)
  enable_testing()

  set(GTEST_PROJECT_NAME gtest_${GTEST_VERSION})
  set(GTEST_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${GTEST_PROJECT_NAME})
  if (MACOS)
    set(GTEST_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    set(GTEST_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
  else()
    set(GTEST_C_FLAGS ${CMAKE_C_FLAGS})
    set(GTEST_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  endif()
  ExternalProject_Add(gtest_project
    DOWNLOAD_NO_PROGRESS 1
    URL https://github.com/google/googletest/archive/refs/tags/${GTEST_VERSION}.tar.gz 
    PREFIX ${GTEST_BUILD_ROOT}
    CMAKE_ARGS
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS=${GTEST_C_FLAGS}
      -DCMAKE_CXX_FLAGS=${GTEST_CXX_FLAGS}
      -DCMAKE_POSITION_INDEPENDENT_CODE=${CMAKE_POSITION_INDEPENDENT_CODE}
      -DCMAKE_BUILD_TYPE=${EXTERNAL_BUILD_TYPE}
    INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
  )

  set(GTEST_INCLUDE_DIRS
    ${GTEST_BUILD_ROOT}/src/gtest_project/googletest/include
    ${GTEST_BUILD_ROOT}/src/gtest_project/googlemock/include
  )

  if(UNIX OR MINGW)
    set(GTEST_LIBRARIES
      ${GTEST_BUILD_ROOT}/src/gtest_project-build/lib/libgmock.a
      ${GTEST_BUILD_ROOT}/src/gtest_project-build/lib/libgtest.a
      ${GTEST_BUILD_ROOT}/src/gtest_project-build/lib/libgtest_main.a
    )
  endif()

  file(GLOB_RECURSE UNITTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mocks/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/utils/*.hpp
  )

  add_project_executable(unittests "${UNITTEST_SOURCES}")
  add_dependencies(unittests 
    gtest_project
    zlib_project 
  )

  target_compile_definitions(unittests PUBLIC 
    GTEST_LINKED_AS_SHARED_LIBRARY=1
    REPERTORY_TESTING
  )
  target_include_directories(unittests PUBLIC
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )
  target_link_libraries(unittests PRIVATE ${GTEST_LIBRARIES})

  add_test(NAME AllTests COMMAND unittests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif()
