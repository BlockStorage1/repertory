if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(IS_CLANG_COMPILER TRUE)
endif()

if (UNIX OR MINGW)
  set(REPERTORY_COMMON_FLAG_LIST
    ${REPERTORY_COMMON_FLAG_LIST}
    -D_FILE_OFFSET_BITS=64
    -march=${REPERTORY_UNIX_ARCH}
    -mtune=generic
  )

  if (MINGW)
    set(REPERTORY_COMMON_FLAG_LIST
      ${REPERTORY_COMMON_FLAG_LIST}
      -Wa,-mbig-obj
      -flarge-source-files
    )
  endif()

  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(REPERTORY_COMMON_FLAG_LIST
      ${REPERTORY_COMMON_FLAG_LIST}
      -DDEBUG
      -D_DEBUG
      -Og
      -fno-omit-frame-pointer
      -g
      -gdwarf-4
    )
  else()
    set(REPERTORY_COMMON_FLAG_LIST
      ${REPERTORY_COMMON_FLAG_LIST}
      -O3
      -DNDEBUG
    )
  endif()

  if (NOT IS_CLANG_COMPILER)
    set(REPERTORY_GCC_FLAGS 
      -Wall
      -Wcast-align
      -Wconversion
      -Wdouble-promotion
      -Wduplicated-branches
      -Wduplicated-cond
      -Wextra
      -Wformat=2
      -Wlogical-op
      -Wmisleading-indentation
      -Wnull-dereference
      -Wpedantic
      -Wshadow
      -Wsign-conversion
      -Wunused
    )

    set(REPERTORY_GCC_CXX_FLAGS 
      -Wnon-virtual-dtor
      -Wold-style-cast
      -Woverloaded-virtual
      -Wuseless-cast
    )
  endif()

  set(REPERTORY_C_FLAGS_LIST
    ${REPERTORY_C_FLAGS_LIST}
    ${REPERTORY_COMMON_FLAG_LIST}
  )

  set(REPERTORY_CXX_FLAGS_LIST
    ${REPERTORY_CXX_FLAGS_LIST}
    ${REPERTORY_COMMON_FLAG_LIST}
    -std=c++${CMAKE_CXX_STANDARD}
  )
elseif (MSVC)
  set(REPERTORY_C_FLAGS_LIST
    ${REPERTORY_C_FLAGS_LIST}
    /D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
    /DNOMINMAX
    /bigobj
    /Zi
  )

  set(REPERTORY_CXX_FLAGS_LIST
    ${REPERTORY_CXX_FLAGS_LIST}
    /D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
    /DNOMINMAX
    /bigobj
    /Zi
  )

  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(REPERTORY_C_FLAGS_LIST
      ${REPERTORY_C_FLAGS_LIST}
      /DEBUG
    )

    set(REPERTORY_CXX_FLAGS_LIST
      ${REPERTORY_CXX_FLAGS_LIST}
      /DEBUG
    )

    set(REPERTORY_SHARED_LINKER_FLAGS_LIST
      ${REPERTORY_SHARED_LINKER_FLAGS_LIST}
      /DEBUG
      /OPT:REF
      /OPT:ICF
    )
  endif()
endif()

list(JOIN REPERTORY_CXX_FLAGS_LIST " " REPERTORY_CXX_FLAGS_LIST)
list(JOIN REPERTORY_C_FLAGS_LIST " " REPERTORY_C_FLAGS_LIST)
list(JOIN REPERTORY_SHARED_LINKER_FLAGS_LIST " " REPERTORY_SHARED_LINKER_FLAGS_LIST)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${REPERTORY_C_FLAGS_LIST}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${REPERTORY_CXX_FLAGS_LIST}")
set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${REPERTORY_SHARED_LINKER_FLAGS_LIST}")
if (ALPINE_FOUND OR MINGW)
  set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
endif()
