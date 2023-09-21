file(GLOB ANALOGICAL_PLUGINS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.h)

add_library(
  analogical_plugins
  SHARED
  ${ANALOGICAL_PLUGINS_HEADERS}
)

add_library(
  ana::plugins ALIAS analogical_plugins
)

set_target_properties(analogical_plugins PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(
  analogical_plugins 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries(
  analogical_plugins
  INTERFACE ana::analogical
)

enable_testing()
add_subdirectory("units")
