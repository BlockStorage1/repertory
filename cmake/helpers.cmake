function(copy_support_files target)
  if (MSVC OR MINGW)
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/3rd_party/winfsp-${WINFSP_VERSION}/bin/${WINFSP_LIBRARY_BASENAME}.dll
        ${REPERTORY_OUTPUT_DIR}/${WINFSP_LIBRARY_BASENAME}.dll)
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_SOURCE_DIR}/3rd_party/cacert.pem
      ${REPERTORY_OUTPUT_DIR}/cacert.pem)
endfunction(copy_support_files)

function(set_common_target_options name)
  target_compile_definitions(${name} PUBLIC ${REPERTORY_DEFINITIONS})

  if (UNIX OR MINGW)
    target_compile_options(${name} PRIVATE 
        ${REPERTORY_GCC_CXX_FLAGS}
        ${REPERTORY_GCC_FLAGS}
    )
  endif()

  target_precompile_headers(${name} PRIVATE include/common.hpp)

  if (MACOS)
    target_link_libraries(${name} PRIVATE -W1,-rpath,@executable_path)
    set_target_properties(${name} PROPERTIES COMPILE_FLAGS -fvisibility=hidden)
  endif()

  if (UNIX OR MINGW)
    target_compile_options(${name} PRIVATE 
      ${REPERTORY_GCC_CXX_FLAGS}
      ${REPERTORY_GCC_FLAGS}
    )
  endif()
endfunction(set_common_target_options)

function(add_project_executable name sources)
  add_executable(${name}
    src/main.cpp
    ${REPERTORY_HEADERS}
    ${sources}
  )

  set_common_target_options(${name})

  add_dependencies(${name} librepertory)

  if (REPERTORY_MUSL OR MINGW)
    set_property(TARGET ${name} PROPERTY LINK_SEARCH_START_STATIC 1)
  endif()

  target_link_libraries(${name} PRIVATE librepertory)

  copy_support_files(${name})
endfunction(add_project_executable)
