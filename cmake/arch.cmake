if("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
  message(FATAL_ERROR "32-bit compilation is not supported")
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*|ARM64.*)")
  set(IS_ARM_SYSTEM 1)
endif()

if(PROJECT_IS_ARM64 AND NOT IS_ARM_SYSTEM AND NOT PROJECT_REQUIRE_ALPINE)
  message(FATAL_ERROR "aarch64 cross-compile is not supported")
endif()

string(REPLACE "_" "-" PROJECT_MARCH ${PROJECT_BUILD_ARCH})
if("${PROJECT_BUILD_ARCH}" STREQUAL "aarch64")
  set(PROJECT_MARCH armv8-a)
endif()
