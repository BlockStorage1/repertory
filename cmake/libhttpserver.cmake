set(LIBHTTPSERVER_PROJECT_NAME libhttpserver_${LIBHTTPSERVER_VERSION})
set(LIBHTTPSERVER_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${LIBHTTPSERVER_PROJECT_NAME})

if (MSVC)
  add_custom_target(libhttpserver_project DEPENDS libmicrohttpd_project)

  include_directories(SYSTEM
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/glue
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libhttpserver-${LIBHTTPSERVER_VERSION}/src
  )
  set(REPERTORY_HEADERS
    ${REPERTORY_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/glue/pthread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/glue/strings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/glue/unistd.h
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/glue/sys/time.h
  )
  set(REPERTORY_SOURCES
    ${REPERTORY_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libhttpserver-${LIBHTTPSERVER_VERSION}/src/details/http_endpoint.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libhttpserver-${LIBHTTPSERVER_VERSION}/src/string_utilities.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libhttpserver-${LIBHTTPSERVER_VERSION}/src/string_response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libhttpserver-${LIBHTTPSERVER_VERSION}/src/http_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libhttpserver-${LIBHTTPSERVER_VERSION}/src/http_resource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libhttpserver-${LIBHTTPSERVER_VERSION}/src/http_request.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libhttpserver-${LIBHTTPSERVER_VERSION}/src/http_response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libhttpserver-${LIBHTTPSERVER_VERSION}/src/webserver.cpp
  )
  add_definitions(-DHTTPSERVER_COMPILATION)
else()
  set (LIBHTTPSERVER_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${EXTERNAL_BUILD_ROOT}/include")
  set (LIBHTTPSERVER_LDFLAGS "-L${EXTERNAL_BUILD_ROOT}/lib")
  set(LIBHTTPSERVER_MAKE make)
  set(LIBHTTPSERVER_SED "sed -i -e s/have_gnutls=\"yes\"/have_gnutls=\"no\"/g ./configure")
  
  if (MACOS)
    set (LIBHTTPSERVER_CXX g++)
    set (LIBHTTPSERVER_CC gcc)
    set(LIBHTTPSERVER_CXX_FLAGS "${LIBHTTPSERVER_CXX_FLAGS} -fvisibility=hidden")
  else()
    set (LIBHTTPSERVER_CXX ${CMAKE_CXX_COMPILER})
    set (LIBHTTPSERVER_CC ${CMAKE_C_COMPILER})
  endif()
    
  ExternalProject_Add(libhttpserver_project
    DOWNLOAD_NO_PROGRESS 1
    BUILD_IN_SOURCE 1
    URL https://github.com/etr/libhttpserver/archive/${LIBHTTPSERVER_VERSION}.tar.gz
    PREFIX ${LIBHTTPSERVER_BUILD_ROOT}
    CONFIGURE_COMMAND ./bootstrap && sh -c "${LIBHTTPSERVER_SED}"
    BUILD_COMMAND mkdir -p build && cd build && ../configure MAKE=${LIBHTTPSERVER_MAKE} 
      CXX=${LIBHTTPSERVER_CXX} CC=${LIBHTTPSERVER_CC} CXXFLAGS=${LIBHTTPSERVER_CXX_FLAGS} 
      LDFLAGS=${LIBHTTPSERVER_LDFLAGS} CPPFLAGS=-fpermissive  --prefix=${EXTERNAL_BUILD_ROOT} 
      --enable-static=yes --enable-shared=yes --disable-examples --disable-doxygen-doc && ${LIBHTTPSERVER_MAKE}
    INSTALL_COMMAND cd build && ${LIBHTTPSERVER_MAKE} install
  )
  set(LIBHTTPSERVER_LIBRARIES ${EXTERNAL_BUILD_ROOT}/lib/libhttpserver.a)
endif()
add_dependencies(libhttpserver_project libmicrohttpd_project)
