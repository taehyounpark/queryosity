cmake_minimum_required(VERSION 3.14)

project(analogical)

OPTION(ANALOGICAL_INSTALL "Install target" ON)
OPTION(ANALOGICAL_DOCS    "Documentation" OFF)
OPTION(ANALOGICAL_TESTS   "Tests" ON)

set(ANALOGICAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(ANALOGICAL_MAJOR_VERSION 0)
set(ANALOGICAL_MINOR_VERSION 1)
set(ANALOGICAL_PATCH_VERSION 0)
set(ANALOGICAL_VERSION
  ${ANALOGICAL_MAJOR_VERSION}.${ANALOGICAL_MINOR_VERSION}.${ANALOGICAL_PATCH_VERSION})

add_library(analogical_analogical INTERFACE)
add_library(analogical::analogical ALIAS analogical_analogical)

set_target_properties(analogical_analogical PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(
  analogical_analogical
  INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_compile_features(analogical_analogical INTERFACE cxx_std_17 )

target_compile_options(
  analogical_analogical INTERFACE 
  -Wall -Werror 
  -Wno-error=unused-variable -Wno-error=uninitialized 
  $<$<CONFIG:Release>:-O3> 
  $<$<CONFIG:RelWithDebInfo>:-O3> 
  $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-ggdb3> 
)

if (ANALOGICAL_INSTALL)
  include(CMakePackageConfigHelpers)
  include(GNUInstallDirs)

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/analogicalConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/analogicalConfig.cmake"
    INSTALL_DESTINATION lib/cmake/analogical
  )

  write_basic_package_version_file(
    analogicalConfigVersion.cmake
    VERSION ${ANALOGICAL_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
  )

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/analogicalConfigVersion.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/analogicalConfig.cmake"
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/analogical)

  install(
    TARGETS analogical_analogical
    EXPORT analogicalTargets
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )

  install(
    DIRECTORY   ${ANALOGICAL_INCLUDE_DIRS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(
    EXPORT analogicalTargets
    FILE   analogicalTargets.cmake
    NAMESPACE analogical::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/analogical
  )

endif()

if (ANALOGICAL_TESTS)
  add_subdirectory("tests")
endif()