cmake_minimum_required(VERSION 3.14)

project(queryosity)


OPTION(ANALOGICAL_INSTALL "Install target" ON)
OPTION(ANALOGICAL_DOCS    "Documentation" OFF)
OPTION(ANALOGICAL_TESTS   "Tests" OFF)
OPTION(ANALOGICAL_EXAMPLES "Install target" OFF)

set(ANALOGICAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(ANALOGICAL_MAJOR_VERSION 0)
set(ANALOGICAL_MINOR_VERSION 1)
set(ANALOGICAL_PATCH_VERSION 0)
set(ANALOGICAL_VERSION ${ANALOGICAL_MAJOR_VERSION}.${ANALOGICAL_MINOR_VERSION}.${ANALOGICAL_PATCH_VERSION})

set(ANALOGICAL_CXX_STANDARD 17 CACHE STRING "Minimum C++ standard") 

add_library(queryosity INTERFACE)
add_library(queryosity::queryosity ALIAS queryosity)

set_target_properties(queryosity PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(
  queryosity
  INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_compile_features(queryosity INTERFACE "cxx_std_${ANALOGICAL_CXX_STANDARD}" )

target_compile_options(
  queryosity INTERFACE 
  -Wall -Wextra
  $<$<CONFIG:Release>:-O3> 
  $<$<CONFIG:RelWithDebInfo>:-O3> 
  $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-ggdb3> 
)

if (ANALOGICAL_INSTALL)
  include(CMakePackageConfigHelpers)
  include(GNUInstallDirs)

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/queryosityConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/queryosityConfig.cmake"
    INSTALL_DESTINATION lib/cmake/queryosity
  )

  write_basic_package_version_file(
    queryosityConfigVersion.cmake
    VERSION ${ANALOGICAL_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
  )

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/queryosityConfigVersion.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/queryosityConfig.cmake"
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/queryosity)

  install(
    TARGETS queryosity
    EXPORT queryosityTargets
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )

  install(
    DIRECTORY   ${ANALOGICAL_INCLUDE_DIRS}/ana
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(
    EXPORT queryosityTargets
    FILE   queryosityTargets.cmake
    NAMESPACE queryosity::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/queryosity
  )

  export(EXPORT queryosityTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/queryosityTargets.cmake"
    NAMESPACE queryosity::
  )

endif()

if (ANALOGICAL_TESTS OR ANALOGICAL_EXAMPLES)
  add_subdirectory("plugins")
  if (ANALOGICAL_TESTS)
    enable_testing()
    add_subdirectory("tests")
  endif()
  if (ANALOGICAL_EXAMPLES)
    add_subdirectory("examples")
  endif()
endif()

