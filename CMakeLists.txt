cmake_minimum_required (VERSION 3.24)

project ("ana")

include(cmake/ProjectIsTopLevel.cmake)
if (PROJECT_IS_TOP_LEVEL)
  set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
  add_subdirectory ("include")
  add_subdirectory ("docs")
else()
  add_subdirectory ("include")
endif()