set(LIBMICROHTTPD_PROJECT_NAME libmicrohttpd_${LIBMICROHTTPD_VERSION})
set(LIBMICROHTTPD_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${LIBMICROHTTPD_PROJECT_NAME})

if (MSVC)
  set (LIBMICROHTTPD_BUILD_TYPE x64)

  ExternalProject_Add(libmicrohttpd_project
    DOWNLOAD_NO_PROGRESS 1
    BUILD_IN_SOURCE 1
    URL https://gnu.freemirror.org/gnu/libmicrohttpd/libmicrohttpd-${LIBMICROHTTPD_VERSION}.tar.gz
    PREFIX ${LIBMICROHTTPD_BUILD_ROOT}
    CONFIGURE_COMMAND echo "Configuring"
    BUILD_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libmicrohttpd/build.cmd "${LIBMICROHTTPD_BUILD_ROOT}\\src\\libmicrohttpd_project" ${CMAKE_BUILD_TYPE}
    INSTALL_COMMAND echo "Skipping Installation"
  )
  include_directories(${LIBMICROHTTPD_BUILD_ROOT}/src/libmicrohttpd_project/w32/VS2019/Output/${LIBMICROHTTPD_BUILD_TYPE})
  link_directories(PRIVATE ${LIBMICROHTTPD_BUILD_ROOT}/src/libmicrohttpd_project/w32/VS2019/Output/${LIBMICROHTTPD_BUILD_TYPE})
  set(LIBMICROHTTPD_LIBRARIES libmicrohttpd${DEBUG_EXTRA3}.lib)
else() 
  if (MACOS)
    set (LIBMICROHTTPD_CXX g++)
    set (LIBMICROHTTPD_CC gcc)
    set (LIBMICROHTTPD_CXX_FLAGS "-I${EXTERNAL_BUILD_ROOT}/include -fvisibility=hidden")
  else()
    set (LIBMICROHTTPD_CXX ${CMAKE_CXX_COMPILER})
    set (LIBMICROHTTPD_CC ${CMAKE_C_COMPILER})
    set (LIBMICROHTTPD_CXX_FLAGS -I${EXTERNAL_BUILD_ROOT}/include)
  endif()
  
  ExternalProject_Add(libmicrohttpd_project
    DOWNLOAD_NO_PROGRESS 1
    BUILD_IN_SOURCE 1
    URL https://gnu.freemirror.org/gnu/libmicrohttpd/libmicrohttpd-${LIBMICROHTTPD_VERSION}.tar.gz
    PREFIX ${LIBMICROHTTPD_BUILD_ROOT}
    CONFIGURE_COMMAND ./configure CXX=${LIBMICROHTTPD_CXX} CC=${LIBMICROHTTPD_CC} 
      CXXFLAGS=${LIBMICROHTTPD_CXX_FLAGS} LDFLAGS=-L${EXTERNAL_BUILD_ROOT}/lib 
      --prefix=${EXTERNAL_BUILD_ROOT} --enable-static=yes --disable-https --disable-examples --disable-doc
    BUILD_COMMAND make
    INSTALL_COMMAND make install
  )
  set(LIBMICROHTTPD_LIBRARIES ${EXTERNAL_BUILD_ROOT}/lib/libmicrohttpd.a)
endif()
