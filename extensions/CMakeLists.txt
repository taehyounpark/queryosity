
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

file(GLOB QUERYOSITY_EXT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/queryosity/*.h)

add_library(
  queryosity_extensions
  SHARED
  ${QUERYOSITY_EXT_HEADERS}
)
set_property(TARGET queryosity_extensions PROPERTY EXPORT_NAME extensions)
add_library(queryosity::extensions ALIAS queryosity_extensions)

target_compile_features(queryosity_extensions INTERFACE "cxx_std_${QUERYOSITY_CXX_STANDARD}" )

set_target_properties(queryosity_extensions PROPERTIES LINKER_LANGUAGE CXX)

target_compile_options(
  queryosity_extensions PUBLIC 
  -Wall -Wextra
  $<$<CONFIG:Release>:-O3> 
  $<$<CONFIG:RelWithDebInfo>:-O3> 
  $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-ggdb3> 
)

target_include_directories(
  queryosity_extensions 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries(
  queryosity_extensions
  INTERFACE queryosity::queryosity
)
