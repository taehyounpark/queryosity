cmake_minimum_required(VERSION 3.14)

project(queryosity)

option(QUERYOSITY_INSTALL "Install target" ON)
option(QUERYOSITY_DOCS "Documentation" OFF)
option(QUERYOSITY_EXTENSIONS "Extensions" OFF)
option(QUERYOSITY_TESTS "Tests" OFF)
option(QUERYOSITY_EXAMPLES "Examples" OFF)

set(QUERYOSITY_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(QUERYOSITY_MAJOR_VERSION 0)
set(QUERYOSITY_MINOR_VERSION 5)
set(QUERYOSITY_PATCH_VERSION 0)
set(QUERYOSITY_VERSION
    ${QUERYOSITY_MAJOR_VERSION}.${QUERYOSITY_MINOR_VERSION}.${QUERYOSITY_PATCH_VERSION}
)

if(QUERYOSITY_TESTS OR QUERYOSITY_EXAMPLES)
  set(QUERYOSITY_EXTENSIONS ON)
endif()

set(QUERYOSITY_CXX_STANDARD
    17
    CACHE STRING "Minimum C++ standard")

add_library(queryosity INTERFACE)
add_library(queryosity::queryosity ALIAS queryosity)

set_target_properties(queryosity PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(
  queryosity INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                       "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

target_compile_features(queryosity
                        INTERFACE "cxx_std_${QUERYOSITY_CXX_STANDARD}")

target_compile_options(
  queryosity
  INTERFACE -Wall -Wextra $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:RelWithDebInfo>:-O3> $<$<CONFIG:Debug>:-O0>
            $<$<CONFIG:Debug>:-ggdb3>)

find_package(Threads REQUIRED)
target_link_libraries(queryosity INTERFACE Threads::Threads)

if(QUERYOSITY_INSTALL)
  include(CMakePackageConfigHelpers)
  include(GNUInstallDirs)

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/queryosityConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/queryosityConfig.cmake"
    INSTALL_DESTINATION lib/cmake/queryosity)

  write_basic_package_version_file(
    queryosityConfigVersion.cmake
    VERSION ${QUERYOSITY_VERSION}
    COMPATIBILITY SameMajorVersion ARCH_INDEPENDENT)

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/queryosityConfigVersion.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/queryosityConfig.cmake"
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/queryosity)

  install(
    TARGETS queryosity
    EXPORT queryosityTargets
    INCLUDES
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

  install(DIRECTORY ${QUERYOSITY_INCLUDE_DIRS}/queryosity
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(
    EXPORT queryosityTargets
    FILE queryosityTargets.cmake
    NAMESPACE queryosity::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/queryosity)

  export(
    EXPORT queryosityTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/queryosityTargets.cmake"
    NAMESPACE queryosity::)

endif()

if(QUERYOSITY_EXTENSIONS
   OR QUERYOSITY_TESTS
   OR QUERYOSITY_EXAMPLES)
  add_subdirectory("extensions")
endif()

if(QUERYOSITY_TESTS)
  enable_testing()
  add_subdirectory("tests")
endif()

if(QUERYOSITY_EXAMPLES)
  add_subdirectory("examples")
endif()
