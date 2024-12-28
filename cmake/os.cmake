if(MSVC)
  message(FATAL_ERROR "MSVC will not be supported")
endif()

if(UNIX AND APPLE)
  message(FATAL_ERROR "Apple is not currently supported")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  message(FATAL_ERROR "FreeBSD is not currently supported")
endif()

if(PROJECT_REQUIRE_ALPINE AND NOT PROJECT_IS_ALPINE AND PROJECT_IS_MINGW AND PROJECT_IS_MINGW_UNIX)
  message(FATAL_ERROR "Project requires Alpine Linux to build")
endif()
