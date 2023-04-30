# register the shared object to include both sources and dictionaries
project(ana)
include_directories("${PROJECT_SOURCE_DIR}")

file(GLOB ana_headers ana/*.h*)
file(GLOB ana_sources src/*.cxx)

# register the shared object
add_library( ana SHARED ${ana_sources} ${ana_headers})

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
  PUBLIC_HEADER "${ana_headers}"
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
configure_file(config.cmake.in
  "${PROJECT_BINARY_DIR}/config.cmake" @ONLY)

# Install the config.cmake
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/config.cmake"
  DESTINATION . COMPONENT dev
  )
  
install(DIRECTORY
  "${CMAKE_CURRENT_BINARY_DIR}/ana"
  DESTINATION lib
  )

message(${CMAKE_CURRENT_BINARY_DIR})

# link everything together at the end
install(
  TARGETS ana
  EXPORT config
  DESTINATION lib
)

set(ANA_PYTHONPATH ${CMAKE_CURRENT_BINARY_DIR})
set(ANA_LD_LIBRARY_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(SETUP ${CMAKE_CURRENT_BINARY_DIR}/setup.sh)

# write setup script
file(WRITE ${SETUP} "#!/bin/bash\n")
file(APPEND ${SETUP} "ulimit -S -s unlimited\n" )
file(APPEND ${SETUP} "export LD_LIBRARY_PATH=\${LD_LIBRARY_PATH}:${ANA_LD_LIBRARY_PATH}\n")
file(APPEND ${SETUP} "export DYLD_LIBRARY_PATH=\${LD_LIBRARY_PATH}:${ANA_LD_LIBRARY_PATH}\n")

