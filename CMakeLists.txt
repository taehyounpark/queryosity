cmake_minimum_required (VERSION 3.24)

project ("ana")

# Add the cmake folder so the FindSphinx module is found
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(DOCUMENTATION_BUILD 1)

if (DOCUMENTATION_BUILD)
  add_subdirectory ("include")
  add_subdirectory ("docs")
else()
  add_subdirectory ("include")
endif()