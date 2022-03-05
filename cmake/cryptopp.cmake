set(CRYPTOPP_PROJECT_NAME cryptopp_${CRYPTOPP_VERSION})
set(CRYPTOPP_BUILD_ROOT ${EXTERNAL_BUILD_ROOT}/builds/${CRYPTOPP_PROJECT_NAME})

if(MSVC)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MTEXTRA Debug)
  endif()
  set (CRYPTOPP_PLATFORM x64)
  ExternalProject_Add(cryptopp_project
    DOWNLOAD_NO_PROGRESS 1
    BUILD_IN_SOURCE 1
    URL https://github.com/weidai11/cryptopp/archive/CRYPTOPP_${CRYPTOPP_VERSION}.tar.gz
    PREFIX ${CRYPTOPP_BUILD_ROOT}
    CONFIGURE_COMMAND 
      ${CMAKE_CURRENT_SOURCE_DIR}/bin/sed -e "s/MultiThreaded</MultiThreadedDLL</g" -e "s/MultiThreaded${MTEXTRA}</MultiThreaded${MTEXTRA}DLL</g" ${CRYPTOPP_BUILD_ROOT}/src/cryptopp_project/cryptlib.vcxproj > ${CRYPTOPP_BUILD_ROOT}/src/cryptopp_project/cryptlib2.vcxproj
    BUILD_COMMAND
      msbuild
      cryptlib2.vcxproj
      /p:Configuration=${CMAKE_BUILD_TYPE}
      /p:Platform=${CRYPTOPP_PLATFORM}
      /p:BuildProjectReferences=false
    INSTALL_COMMAND cmd.exe /c echo "Skipping Install"
  )

  include_directories(SYSTEM ${CRYPTOPP_BUILD_ROOT}/src/cryptopp_project)
  link_libraries(${CRYPTOPP_BUILD_ROOT}/src/cryptopp_project/${CRYPTOPP_PLATFORM}/Output/${CMAKE_BUILD_TYPE}/cryptlib.lib)
else()
  set (CRYPTOPP_CXXFLAGS "${CMAKE_CXX_FLAGS} -DCRYPTOPP_DISABLE_ASM")
  set(CRYPTOPP_MAKE_EXEC make)
  if (MACOS)
    set(CRYPTOPP_CXXFLAGS "${CRYPTOPP_CXXFLAGS} -fvisibility=hidden")
    if (EXISTS /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/)
      set(CRYPTOPP_CXXFLAGS "${CRYPTOPP_CXXFLAGS} -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
    endif()
  endif()
  ExternalProject_Add(cryptopp_project
    DOWNLOAD_NO_PROGRESS 1
    BUILD_IN_SOURCE 1
    URL https://github.com/weidai11/cryptopp/archive/CRYPTOPP_${CRYPTOPP_VERSION}.tar.gz
    PREFIX ${CRYPTOPP_BUILD_ROOT}
    CONFIGURE_COMMAND echo "Skipping Configure"
    BUILD_COMMAND
      ${CRYPTOPP_MAKE_EXEC}
      CXXFLAGS=${CRYPTOPP_CXXFLAGS}
      PREFIX=${EXTERNAL_BUILD_ROOT}
      CC=${CMAKE_C_COMPILER}
      CXX=${CMAKE_CXX_COMPILER}
    INSTALL_COMMAND ${CRYPTOPP_MAKE_EXEC} install PREFIX=${EXTERNAL_BUILD_ROOT}
  )
  link_libraries(${EXTERNAL_BUILD_ROOT}/lib/libcryptopp.a)
endif()
