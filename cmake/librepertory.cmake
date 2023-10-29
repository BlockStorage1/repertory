if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/librepertory${CMAKE_STATIC_LIBRARY_SUFFIX}")
  file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/librepertory${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_library(librepertory STATIC 
  ${REPERTORY_SOURCES}
  ${REPERTORY_HEADERS}
)

set_common_target_options(librepertory)

set_target_properties(librepertory PROPERTIES PREFIX "")
target_link_libraries(librepertory PRIVATE ${REPERTORY_LINK_LIBRARIES}) 

add_dependencies(librepertory
  boost_project
  curl_project
  libsodium_project
  rocksdb_project
)

if (LINUX)
  add_dependencies(librepertory libuuid_project)
endif()

if (LINUX OR MINGW OR MACOS)
  add_dependencies(librepertory openssl_project)
endif()

if (MSVC OR MINGW)
  add_dependencies(librepertory zlib_project)
endif()
