
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

file(GLOB QUERYOSITY_PLUGINS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/queryosity/*.h)

add_library(
  queryosity_plugins
  SHARED
  ${QUERYOSITY_PLUGINS_HEADERS}
)
set_property(TARGET queryosity_plugins PROPERTY EXPORT_NAME plugins)
add_library(queryosity::plugins ALIAS queryosity_plugins)

target_compile_features(queryosity_plugins INTERFACE "cxx_std_${QUERYOSITY_CXX_STANDARD}" )

set_target_properties(queryosity_plugins PROPERTIES LINKER_LANGUAGE CXX)

target_compile_options(
  queryosity_plugins PUBLIC 
  -Wall -Wextra
  $<$<CONFIG:Release>:-O3> 
  $<$<CONFIG:RelWithDebInfo>:-O3> 
  $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-ggdb3> 
)

target_include_directories(
  queryosity_plugins 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries(
  queryosity_plugins
  INTERFACE queryosity::queryosity
)
