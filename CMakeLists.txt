################################################################################
# ana
################################################################################

project(ana)
include_directories("${PROJECT_SOURCE_DIR}")

file(GLOB anaHeaders ana/*.h*)
file(GLOB anaSources src/*.cxx)

if(ATLAS_BUILD OR NOT STANDALONE_BUILD)
  include(./cmake/asg.cmake)
endif()

if(STANDALONE_BUILD OR NOT ATLAS_BUILD)
  include(./cmake/standalone.cmake)
endif()

# link everything together at the end
install(
  TARGETS ana
  EXPORT anaConfig
  DESTINATION lib
)

set(SETUP ${CMAKE_CURRENT_BINARY_DIR}/setup.sh)
file(WRITE ${SETUP} "#!/bin/bash\n")
file(APPEND ${SETUP} "ulimit -S -s unlimited\n" )
file(APPEND ${SETUP} "export LD_LIBRARY_PATH=\${LD_LIBRARY_PATH}:${ANA_LD_LIBRARY_PATH}\n")
file(APPEND ${SETUP} "export DYLD_LIBRARY_PATH=\${LD_LIBRARY_PATH}:${ANA_LD_LIBRARY_PATH}\n")