project ("ana")

OPTION(BUILD_DOCS "build documentation" OFF)
OPTION(BUILD_TESTS "build tests" OFF)

add_subdirectory ("include")

if (BUILD_DOCS)
  set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
  add_subdirectory("docs")
endif()

if (BUILD_TESTS)
  add_subdirectory("tests")
endif()