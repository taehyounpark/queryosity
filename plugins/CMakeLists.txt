
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

file(GLOB ANALOGICAL_PLUGINS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)


add_library(
  analogical_plugins
  SHARED
  ${ANALOGICAL_PLUGINS_HEADERS}
)
set_property(TARGET analogical_plugins PROPERTY EXPORT_NAME plugins)
add_library(
  ana::plugins ALIAS analogical_plugins
)

target_compile_features(analogical_plugins INTERFACE "cxx_std_${ANALOGICAL_CXX_STANDARD}" )

set_target_properties(analogical_plugins PROPERTIES LINKER_LANGUAGE CXX)

target_compile_options(
  analogical_plugins PUBLIC 
  -Wall -Wextra
  $<$<CONFIG:Release>:-O3> 
  $<$<CONFIG:RelWithDebInfo>:-O3> 
  $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-ggdb3> 
)

target_include_directories(
  analogical_plugins 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries(
  analogical_plugins
  INTERFACE ana::analogical
)
