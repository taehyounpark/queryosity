# register the shared object to include both sources and dictionaries
include_directories ("${PROJECT_SOURCE_DIR}")

# register the shared object
add_library( ana SHARED ${anaSources} ${anaHeaders})

# link everything together at the end
target_include_directories(
  ana
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set_target_properties(
  ana
  PROPERTIES
  VERSION 0.1
  PUBLIC_HEADER "${anaHeaders}"
)
target_compile_features(ana PRIVATE cxx_std_17)
target_link_libraries( ana Threads::Threads )

# Add all targets to the build-tree export set
export(TARGETS ana FILE "${PROJECT_BINARY_DIR}/anaTargets.cmake")

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE ana)

set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/ana" )
set(CONF_LIBRARY_DIRS "${PROJECT_BINARY_DIR}")
set(CONF_LIBRARIES    ana)
configure_file(anaConfig.cmake.in
  "${PROJECT_BINARY_DIR}/anaConfig.cmake" @ONLY)

# Install the anaConfig.cmake
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/anaConfig.cmake"
  DESTINATION . COMPONENT dev
  )
  
install(DIRECTORY
  "${CMAKE_CURRENT_BINARY_DIR}/ana"
  DESTINATION lib
  )

message(${CMAKE_CURRENT_BINARY_DIR})

set(ANA_PYTHONPATH ${CMAKE_CURRENT_BINARY_DIR})
set(ANA_LD_LIBRARY_PATH ${CMAKE_CURRENT_BINARY_DIR})

