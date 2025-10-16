if(MSVC)
  message(FATAL_ERROR "MSVC will not be supported")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  message(FATAL_ERROR "FreeBSD is not currently supported")
endif()

if(PROJECT_REQUIRE_ALPINE AND NOT PROJECT_IS_ALPINE AND PROJECT_IS_MINGW AND PROJECT_IS_MINGW_UNIX)
  message(FATAL_ERROR "Project requires Alpine Linux to build")
endif()

if (PROJECT_IS_DARWIN)
  if (PROJECT_IS_ARM64)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
  else()
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
  endif()
endif()

if (PROJECT_IS_DARWIN AND NOT PROJECT_MACOS_BUNDLE_ID)
  message(FATAL_ERROR "'PROJECT_MACOS_BUNDLE_ID' is not set in 'config.sh'")
endif()
