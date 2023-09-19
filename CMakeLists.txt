project(analogical VERSION 1.0.0 LANGUAGES CXX)

OPTION(ANALOGICAL_BuildDocs  "Build documentation" OFF)
OPTION(ANALOGICAL_BuildTests "Build tests" ON)

add_subdirectory("include")

if (ANALOGICAL_BuildTests)
  add_subdirectory("tests")
endif()