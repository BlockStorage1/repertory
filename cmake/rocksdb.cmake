set(ROCKSDB_PROJECT_NAME rocksdb_${ROCKSDB_VERSION})
set(ROCKSDB_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${ROCKSDB_PROJECT_NAME})

if (MACOS)
  set(ROCKSDB_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
else()
  set(ROCKSDB_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

set(ROCKSDB_CMAKE_ARGS
  -DCMAKE_BUILD_TYPE=${EXTERNAL_BUILD_TYPE}
  -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
  -DCMAKE_CXX_FLAGS=${ROCKSDB_CMAKE_CXX_FLAGS}
  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
  -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
  -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
  -DCMAKE_INSTALL_PREFIX=${EXTERNAL_BUILD_ROOT}
  -DCMAKE_POSITION_INDEPENDENT_CODE=${CMAKE_POSITION_INDEPENDENT_CODE}
  -DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}
  -DFAIL_ON_WARNINGS=OFF
  -DPORTABLE=1
  -DROCKSDB_BUILD_SHARED=OFF
  -DWITH_BENCHMARK_TOOLS=OFF
  -DWITH_LIBURING=OFF
  -DWITH_TESTS=OFF
  -DWITH_TOOLS=OFF
)

if(MSVC)
  ExternalProject_Add(rocksdb_project
    DOWNLOAD_NO_PROGRESS 1
    URL https://github.com/facebook/rocksdb/archive/v${ROCKSDB_VERSION}.tar.gz
    PREFIX ${ROCKSDB_BUILD_ROOT}
    CMAKE_ARGS ${ROCKSDB_CMAKE_ARGS} 
    INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
  )

  set(ROCKSDB_INCLUDE_DIRS ${ROCKSDB_BUILD_ROOT}/src/rocksdb_project/include)

  if (CMAKE_GENERATOR_LOWER STREQUAL "nmake makefiles")
    set(ROCKSDB_LIBRARIES ${ROCKSDB_BUILD_ROOT}/src/rocksdb_project-build/rocksdb.lib)
  else ()
    set(ROCKSDB_LIBRARIES ${ROCKSDB_BUILD_ROOT}/src/rocksdb_project-build/${CMAKE_BUILD_TYPE}/rocksdb.lib)
  endif ()
else()
  if (MINGW)
    if (CMAKE_TOOLCHAIN_FILE)
      set(ROCKSDB_CMAKE_ARGS
        ${ROCKSDB_CMAKE_ARGS}
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} 
      )
    endif()

    ExternalProject_Add(rocksdb_project
      DOWNLOAD_NO_PROGRESS 1
      URL https://github.com/facebook/rocksdb/archive/v${ROCKSDB_VERSION}.tar.gz
      PREFIX ${ROCKSDB_BUILD_ROOT}
      CMAKE_ARGS ${ROCKSDB_CMAKE_ARGS} -DWITH_GFLAGS=OFF
      INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
    )

    set(ROCKSDB_LIBRARIES ${ROCKSDB_BUILD_ROOT}/src/rocksdb_project-build/librocksdb.a)
    include_directories(SYSTEM ${ROCKSDB_BUILD_ROOT}/src/rocksdb_project/include)
  else()
    ExternalProject_Add(rocksdb_project
      DOWNLOAD_NO_PROGRESS 1
      URL https://github.com/facebook/rocksdb/archive/v${ROCKSDB_VERSION}.tar.gz
      PREFIX ${ROCKSDB_BUILD_ROOT}
      CMAKE_ARGS ${ROCKSDB_CMAKE_ARGS} -DWITH_GFLAGS=OFF
    )

    if (MACOS)
      set(ROCKSDB_LIBRARIES ${EXTERNAL_BUILD_ROOT}/lib/librocksdb.a)
    else()
      set(ROCKSDB_LIBRARIES librocksdb.a)
    endif()
  endif()
endif()

if (MSVC OR LINUX OR MINGW)
  add_dependencies(rocksdb_project curl_project)
endif()
