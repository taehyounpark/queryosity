cmake_minimum_required(VERSION 3.12)

project(analogical 
  VERSION 0.1.0
  DESCRIPTION "Coherent data dataflow interface in C++"
  HOMEPAGE_URL "https://github.com/taehyounpark/analogical.git"
  LANGUAGES CXX
)

OPTION(ANALOGICAL_BuildDocs  "Build documentation" OFF)
OPTION(ANALOGICAL_BuildTests "Build tests" ON)

add_subdirectory("include")

if (ANALOGICAL_BuildTests)
  add_subdirectory("tests")
endif()