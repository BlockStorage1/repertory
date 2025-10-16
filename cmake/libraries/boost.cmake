if(PROJECT_ENABLE_BOOST)
  if(PROJECT_ENABLE_LIBBITCOIN_SYSTEM)
    set(BOOST_MAJOR_VERSION ${BOOST2_MAJOR_VERSION})
    set(BOOST_MINOR_VERSION ${BOOST2_MINOR_VERSION})
    set(BOOST_PATCH_VERSION ${BOOST2_PATCH_VERSION})
  endif()

  set(BOOST_ROOT ${PROJECT_EXTERNAL_BUILD_ROOT})
  set(BOOST_INCLUDE_DIR ${BOOST_ROOT}/include/boost-${BOOST_MINOR_VERSION}_${BOOST_MINOR_VERSION})

  set(PROJECT_EXTERNAL_CMAKE_FLAGS
    ${PROJECT_EXTERNAL_CMAKE_FLAGS}
    -DBUILD_SHARED_LIBS=${PROJECT_BUILD_SHARED_LIBS}
    -DBOOST_INCLUDE_DIR=${BOOST_INCLUDE_DIR}
    -DBOOST_ROOT=${BOOST_ROOT}
  )

  if(PROJECT_BUILD)
    add_definitions(-DPROJECT_ENABLE_BOOST)

    if (PROJECT_ENABLE_LIBBITCOIN_SYSTEM)
      find_package(Boost ${BOOST_MAJOR_VERSION}.${BOOST_MINOR_VERSION}.${BOOST_PATCH_VERSION} EXACT
        REQUIRED
        COMPONENTS
          atomic
          chrono
          date_time
          filesystem
          iostreams
          locale
          log
          log_setup
          program_options
          random
          regex
          serialization
          system
          thread
          wserialization
      )
    else()
      if(PROJECT_IS_DARWIN)
        set(CMAKE_HAVE_THREADS_LIBRARY 1)
        set(CMAKE_THREAD_LIBS_INIT "-lpthread")
        set(CMAKE_USE_PTHREADS_INIT 1)
        set(CMAKE_USE_WIN32_THREADS_INIT 0)
        set(THREADS_PREFER_PTHREAD_FLAG ON)
      endif()

      find_package(Boost ${BOOST_MAJOR_VERSION}.${BOOST_MINOR_VERSION}.${BOOST_PATCH_VERSION}
        REQUIRED
        COMPONENTS
          atomic
          chrono
          date_time
          filesystem
          iostreams
          locale
          log
          log_setup
          program_options
          random
          regex
          serialization
          thread
          wserialization
      )
    endif()

    include_directories(BEFORE SYSTEM
      ${Boost_INCLUDE_DIRS}
    )

    link_libraries(
      ${Boost_LIBRARIES}
    )
  elseif(NOT PROJECT_IS_MINGW OR CMAKE_HOST_WIN32)
    if (PROJECT_ENABLE_LIBBITCOIN_SYSTEM OR NOT CMAKE_HOST_WIN32)
      if(PROJECT_CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(BOOST_BUILD_TYPE_LOWER debug)
      else()
        set(BOOST_BUILD_TYPE_LOWER release)
      endif()

      if(PROJECT_IS_ARM64)
        set(BOOST_ARCH arm)
      else()
        set(BOOST_ARCH x86)
      endif()

      if(PROJECT_STATIC_LINK)
        set(BOOST_LINK static)
      else()
        set(BOOST_LINK "static,shared")
      endif()

      set(BOOST_BUILD_ARGS
        --openssldir=$ENV{OPENSSL_ROOT_DIR}
        --prefix=${PROJECT_EXTERNAL_BUILD_ROOT}
        address-model=64
        architecture=${BOOST_ARCH}
        cxxstd=20
        cxxstd-dialect=gnu
        cxxflags=-std=gnu++${CMAKE_CXX_STANDARD}
        cxxstd=${CMAKE_CXX_STANDARD}
        define=BOOST_ASIO_HAS_STD_STRING_VIEW
        define=BOOST_SYSTEM_NO_DEPRECATED
        link=${BOOST_LINK}
        linkflags=-std=gnu++${CMAKE_CXX_STANDARD}
        threading=multi
        variant=${BOOST_BUILD_TYPE_LOWER}
      )

      if(PROJECT_ENABLE_LIBBITCOIN_SYSTEM)
        set(BOOST_URL_HASH SHA256=${BOOST2_HASH})
      else()
        set(BOOST_URL_HASH SHA256=${BOOST_HASH})
      endif()

      ExternalProject_Add(boost_project
        PREFIX external
        URL ${PROJECT_3RD_PARTY_DIR}/boost_${BOOST_MAJOR_VERSION}_${BOOST_MINOR_VERSION}_${BOOST_PATCH_VERSION}.tar.gz
        URL_HASH ${BOOST_URL_HASH}
        BUILD_IN_SOURCE 1
        LIST_SEPARATOR |
        CONFIGURE_COMMAND ./bootstrap.sh
          --with-libraries=atomic,chrono,date_time,filesystem,iostreams,locale,log,program_options,random,regex,serialization,system,test,thread
        BUILD_COMMAND
          ./b2
          -sNO_BZIP2=1
          -j$ENV{CMAKE_BUILD_PARALLEL_LEVEL}
          ${BOOST_BUILD_ARGS}
        INSTALL_COMMAND
          ./b2
          -sNO_BZIP2=1
          -j$ENV{CMAKE_BUILD_PARALLEL_LEVEL}
          ${BOOST_BUILD_ARGS}
          install
      )

      list(APPEND PROJECT_DEPENDENCIES boost_project)

     if(PROJECT_IS_DARWIN OR PROJECT_REQUIRE_ALPINE)
        add_dependencies(boost_project icu_project)
      endif()

      if (NOT CMAKE_HOST_WIN32)
        add_dependencies(boost_project openssl_project)
      endif()
    endif()
  endif()
endif()
