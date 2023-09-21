cmake_minimum_required(VERSION 3.14)

project(analogical)


OPTION(ANALOGICAL_INSTALL "Install target" OFF)
OPTION(ANALOGICAL_DOCS    "Documentation" OFF)
OPTION(ANALOGICAL_TESTS   "Tests" OFF)

set(ANALOGICAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(ANALOGICAL_MAJOR_VERSION 0)
set(ANALOGICAL_MINOR_VERSION 1)
set(ANALOGICAL_PATCH_VERSION 0)
set(ANALOGICAL_VERSION ${ANALOGICAL_MAJOR_VERSION}.${ANALOGICAL_MINOR_VERSION}.${ANALOGICAL_PATCH_VERSION})

set(ANALOGICAL_CXX_STANDARD 17 CACHE STRING "Minimum C++ standard") 

add_library(analogical INTERFACE)
add_library(ana::analogical ALIAS analogical)

set_target_properties(analogical PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(
  analogical
  INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_compile_features(analogical INTERFACE "cxx_std_${ANALOGICAL_CXX_STANDARD}" )

target_compile_options(
  analogical INTERFACE 
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
    TARGETS analogical
    EXPORT analogicalTargets
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )

  install(
    DIRECTORY   ${ANALOGICAL_INCLUDE_DIRS}/ana
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(
    EXPORT analogicalTargets
    FILE   analogicalTargets.cmake
    NAMESPACE ana::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/analogical
  )

  export(EXPORT analogicalTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/analogicalTargets.cmake"
    NAMESPACE ana::
  )

endif()

if (ANALOGICAL_TESTS)
  add_subdirectory("plugins")
  enable_testing()
  add_subdirectory("tests")
endif()