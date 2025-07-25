if(PROJECT_ENABLE_WINFSP AND PROJECT_IS_MINGW)
  if(PROJECT_BUILD) 
    add_definitions(-DPROJECT_ENABLE_WINFSP)

    include_directories(BEFORE SYSTEM ${PROJECT_3RD_PARTY_DIR}/winfsp-2.1/inc)

    link_directories(BEFORE ${PROJECT_3RD_PARTY_DIR}/winfsp-2.1/lib)

    if(PROJECT_IS_ARM64)
      link_libraries(winfsp-a64)
    else()
      link_libraries(winfsp-x64)
    endif()
  endif()
endif()
