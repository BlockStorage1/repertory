# Testing
if (REPERTORY_ENABLE_TESTING)
  enable_testing()

  set(GTEST_PROJECT_NAME gtest_${GTEST_VERSION})
  set(GTEST_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${GTEST_PROJECT_NAME})
  if (MSVC)
    ExternalProject_Add(gtest_project
      DOWNLOAD_NO_PROGRESS 1
      URL https://github.com/google/googletest/archive/release-${GTEST_VERSION}.tar.gz
      PREFIX ${GTEST_BUILD_ROOT}
      CMAKE_ARGS
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_BUILD_TYPE=${EXTERNAL_BUILD_TYPE}
        -Dgtest_force_shared_crt=OFF
        -DBUILD_SHARED_LIBS=ON
        -DCMAKE_CXX_FLAGS=/D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
      INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
      )
  else()
    ExternalProject_Add(gtest_project
      DOWNLOAD_NO_PROGRESS 1
      URL https://github.com/google/googletest/archive/release-${GTEST_VERSION}.tar.gz
      PREFIX ${GTEST_BUILD_ROOT}
      CMAKE_ARGS
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_BUILD_TYPE=${EXTERNAL_BUILD_TYPE}
      INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
    )
  endif()

  set(GTEST_INCLUDE_DIRS
    ${GTEST_BUILD_ROOT}/src/gtest_project/googletest/include
    ${GTEST_BUILD_ROOT}/src/gtest_project/googlemock/include
  )

  if (MSVC)
    if (NOT CMAKE_GENERATOR_LOWER STREQUAL "nmake makefiles")
      set (GTEST_PATH_EXTRA ${CMAKE_BUILD_TYPE}/)
    endif()
    set(GTEST_LIBRARIES
      ${GTEST_BUILD_ROOT}/src/gtest_project-build/lib/${GTEST_PATH_EXTRA}gmock${DEBUG_EXTRA}.lib
      ${GTEST_BUILD_ROOT}/src/gtest_project-build/lib/${GTEST_PATH_EXTRA}gmock_main${DEBUG_EXTRA}.lib
      ${GTEST_BUILD_ROOT}/src/gtest_project-build/lib/${GTEST_PATH_EXTRA}gtest${DEBUG_EXTRA}.lib
      ${GTEST_BUILD_ROOT}/src/gtest_project-build/lib/${GTEST_PATH_EXTRA}gtest_main${DEBUG_EXTRA}.lib
    )
  elseif (UNIX)
    set(GTEST_LIBRARIES
      ${GTEST_BUILD_ROOT}/src/gtest_project-build/lib/libgmock${DEBUG_EXTRA}.a
      ${GTEST_BUILD_ROOT}/src/gtest_project-build/lib/libgtest${DEBUG_EXTRA}.a
      ${GTEST_BUILD_ROOT}/src/gtest_project-build/lib/libgtest_main${DEBUG_EXTRA}.a
    )
  endif()

  file(GLOB_RECURSE UNITTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mocks/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/utils/*.hpp
  )

  add_executable(unittests
    src/main.cpp
    ${REPERTORY_HEADERS}
    ${UNITTEST_SOURCES}
  )

  #target_precompile_headers(unittests PRIVATE include/common.hpp tests/test_common.hpp)
  target_compile_definitions(unittests PUBLIC GTEST_LINKED_AS_SHARED_LIBRARY=1 REPERTORY_TESTING)

  target_link_libraries(unittests PRIVATE ${GTEST_LIBRARIES} librepertory)

  if (MSVC)
    add_custom_command(TARGET unittests
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${GTEST_BUILD_ROOT}/src/gtest_project-build/bin/${GTEST_PATH_EXTRA}gmock${DEBUG_EXTRA}.dll ${CMAKE_CURRENT_BINARY_DIR}/${GTEST_PATH_EXTRA}
      )
    add_custom_command(TARGET unittests
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${GTEST_BUILD_ROOT}/src/gtest_project-build/bin/${GTEST_PATH_EXTRA}gmock_main${DEBUG_EXTRA}.dll ${CMAKE_CURRENT_BINARY_DIR}/${GTEST_PATH_EXTRA}
      )
    add_custom_command(TARGET unittests
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${GTEST_BUILD_ROOT}/src/gtest_project-build/bin/${GTEST_PATH_EXTRA}gtest${DEBUG_EXTRA}.dll ${CMAKE_CURRENT_BINARY_DIR}/${GTEST_PATH_EXTRA}
      )
    add_custom_command(TARGET unittests
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${GTEST_BUILD_ROOT}/src/gtest_project-build/bin/${GTEST_PATH_EXTRA}gtest_main${DEBUG_EXTRA}.dll ${CMAKE_CURRENT_BINARY_DIR}/${GTEST_PATH_EXTRA}
      )
  endif()
  target_include_directories(unittests PUBLIC
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

  add_custom_command(
    TARGET unittests
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/cacert.pem
    ${CMAKE_BINARY_DIR}/cacert.pem
  )

  add_dependencies(unittests
    gtest_project
    librepertory
  )

  if (MACOS)
    target_link_libraries(unittests PRIVATE -W1,-rpath,@executable_path)
    set_target_properties(unittests PROPERTIES COMPILE_FLAGS -fvisibility=hidden)
  elseif (MSVC)
    CopySupportFiles(unittests)
  endif()

  if (CMAKE_GENERATOR_LOWER STREQUAL "nmake makefiles")
    add_test(NAME AllTests COMMAND unittests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  else()
    add_test(NAME AllTests COMMAND unittests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
  endif()
endif()
