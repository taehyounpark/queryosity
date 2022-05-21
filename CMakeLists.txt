################################################################################
# ana
################################################################################

project(ana)
include_directories("${PROJECT_SOURCE_DIR}")

file(GLOB anaHeaders ana/*.h*)
file(GLOB anaSources src/*.cxx)

if(ATLAS_BUILD OR NOT STANDALONE_BUILD)
  include(cmake/asg.cmake)
endif()

if(STANDALONE_BUILD OR NOT ATLAS_BUILD)
  include(cmake/standalone.cmake)
endif()

#include(cmake/standalone.cmake)

# link everything together at the end
install(
  TARGETS ana
  EXPORT anaConfig
  DESTINATION lib
)