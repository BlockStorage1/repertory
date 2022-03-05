if (REPERTORY_ENABLE_S3)
  set(AWSCPP_PROJECT_NAME awscpp_${AWSCPP_VERSION})
  set(AWSCPP_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${AWSCPP_PROJECT_NAME})

  if (LINUX)
    if (IS_CLANG_COMPILER)
      set(AWSCPP_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-function -Wno-error=uninitialized")
      set(AWSCPP_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=unused-function -Wno-error=uninitialized")
    else ()
      set(AWSCPP_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-function -Wno-error=maybe-uninitialized")
      set(AWSCPP_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=unused-function -Wno-error=maybe-uninitialized")
    endif ()
  elseif (MACOS)
    set(AWSCPP_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
  else ()
    set(AWSCPP_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  endif ()

  set(AWSCPP_CMAKE_ARGS
    -DAWS_WARNINGS_ARE_ERRORS=OFF
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
    -DCMAKE_CXX_FLAGS=${AWSCPP_CXX_FLAGS}
    -DCMAKE_C_FLAGS=${AWSCPP_C_FLAGS}
    -DCMAKE_BUILD_TYPE=${EXTERNAL_BUILD_TYPE}
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}
    -DCMAKE_INSTALL_PREFIX=${EXTERNAL_BUILD_ROOT}
    -DBUILD_ONLY=s3
    -DBUILD_SHARED_LIBS=OFF
    -DENABLE_TESTING=OFF
    -DOPENSSL_USE_STATIC_LIBS=ON
    -DCPP_STANDARD=${CMAKE_CXX_STANDARD}
    -DCURL_LIBRARY=${CURL_LIBRARIES}
    )

  if (OPENSSL_ROOT_DIR)
    set(AWSCPP_CMAKE_ARGS ${AWSCPP_CMAKE_ARGS} -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR})
  elseif (LINUX)
    set(AWSCPP_CMAKE_ARGS ${AWSCPP_CMAKE_ARGS} -DOPENSSL_ROOT_DIR=${EXTERNAL_BUILD_ROOT})
  endif ()

  ExternalProject_Add(awscpp_project
    DOWNLOAD_NO_PROGRESS 1
    GIT_REPOSITORY https://github.com/aws/aws-sdk-cpp.git
    GIT_TAG ${AWSCPP_VERSION}
    GIT_SUBMODULES_RECURSE ON
    PREFIX ${AWSCPP_BUILD_ROOT}
    CMAKE_ARGS ${AWSCPP_CMAKE_ARGS}
    # BUILD_COMMAND echo 0
    # CONFIGURE_COMMAND echo 0
    # INSTALL_COMMAND echo 0
  )

  if (MSVC OR LINUX)
    add_dependencies(awscpp_project curl_project)
  endif ()

  if (MACOS OR LINUX)
    add_dependencies(awscpp_project openssl_project)
  endif ()

  if (MSVC)
    set(AWSCPP_LIBRARIES
      ${EXTERNAL_BUILD_ROOT}/lib/aws-cpp-sdk-s3.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-cpp-sdk-core.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-crt-cpp.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-c-s3.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-c-auth.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-c-mqtt.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-c-http.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-c-event-stream.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-c-io.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-checksums.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-c-cal.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-c-compression.lib
      ${EXTERNAL_BUILD_ROOT}/lib/aws-c-common.lib
    )
  else ()
    set(AWSCPP_LIBRARIES
      libaws-cpp-sdk-s3.a
      libaws-cpp-sdk-core.a
      libaws-crt-cpp.a
      libaws-c-s3.a
      libaws-c-auth.a
      libaws-c-mqtt.a
      libaws-c-http.a
      libaws-c-event-stream.a
      libaws-c-io.a
      libs2n.a
      libaws-checksums.a
      libaws-c-cal.a
      libaws-c-compression.a
      libaws-c-common.a
    )
  endif ()
endif()
