cmake_minimum_required(VERSION 3.14)

project(queryosity)

option(QUERYOSITY_INSTALL "Install target" OFF)
option(QUERYOSITY_DOCS "Documentation" OFF)

option(QUERYOSITY_JSON  "Extension: nlohmann::json" OFF)
option(QUERYOSITY_CSV   "Extension: rapidcsv" OFF)
option(QUERYOSITY_BOOST "Extension: boost::histogram" OFF)
option(QUERYOSITY_ROOT  "Extension: ROOT" OFF)
set(QUERYOSITY_EXTENSIONS OFF)
if(QUERYOSITY_JSON OR QUERYOSITY_BOOST OR QUERYOSITY_CSV OR QUERYOSITY_ROOT)
    set(QUERYOSITY_EXTENSIONS ON)
endif()

option(QUERYOSITY_TESTS "Tests" OFF)
option(QUERYOSITY_EXAMPLES "Examples" OFF)

set(QUERYOSITY_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(QUERYOSITY_MAJOR_VERSION 0)
set(QUERYOSITY_MINOR_VERSION 9)
set(QUERYOSITY_PATCH_VERSION 1)
set(QUERYOSITY_VERSION
    ${QUERYOSITY_MAJOR_VERSION}.${QUERYOSITY_MINOR_VERSION}.${QUERYOSITY_PATCH_VERSION}
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(QUERYOSITY_TESTS) 
  set(QUERYOSITY_JSON ON)
  set(QUERYOSITY_CSV ON)
endif()

if (QUERYOSITY_EXAMPLES)
  set(QUERYOSITY_JSON ON)
  set(QUERYOSITY_CSV ON)
  set(QUERYOSITY_BOOST ON)
  set(QUERYOSITY_ROOT ON)
endif()

add_library(queryosity INTERFACE)
add_library(queryosity::queryosity ALIAS queryosity)

set_target_properties(queryosity PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(
  queryosity INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                       "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

target_compile_options(
  queryosity
  INTERFACE -Wall -Wextra -Winvalid-pch $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:RelWithDebInfo>:-O3> $<$<CONFIG:Debug>:-O0>
            $<$<CONFIG:Debug>:-ggdb3>)

find_package(Threads REQUIRED)
target_link_libraries(queryosity INTERFACE Threads::Threads)

if(QUERYOSITY_INSTALL)
  include(CMakePackageConfigHelpers)
  include(GNUInstallDirs)

  # 1) Generate the config + version file
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/queryosityConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/queryosityConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/queryosity
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/queryosityConfigVersion.cmake"
    VERSION ${QUERYOSITY_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  # 2) Install config + version files
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/queryosityConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/queryosityConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/queryosity
  )

  # 3) Install headers
  install(DIRECTORY ${QUERYOSITY_INCLUDE_DIRS}/queryosity
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  # 4) Install target + export file
  install(
    TARGETS queryosity
    EXPORT queryosityTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(
    EXPORT queryosityTargets
    FILE queryosityTargets.cmake
    NAMESPACE queryosity::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/queryosity
  )

else()  # ====== BUILD-TREE USAGE ONLY ======

  export(PACKAGE queryosity)

  # # 1) Export targets locally
  # export(
  #   EXPORT queryosityTargets
  #   FILE "${PROJECT_BINARY_DIR}/queryosityTargets.cmake"
  #   NAMESPACE queryosity::
  # )

  # 2) Configure a local config file that references the above
  set(CONF_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")
  set(CONF_LIBRARY_DIRS "${PROJECT_BINARY_DIR}")
  set(CONF_LIBRARIES    queryosity)

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/queryosityConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/queryosityConfig.cmake"
    @ONLY
  )

endif()

if(QUERYOSITY_EXTENSIONS)
  add_subdirectory("extensions")
endif()

if(QUERYOSITY_TESTS)
  enable_testing()
  add_subdirectory("tests")
endif()

if(QUERYOSITY_EXAMPLES)
  add_subdirectory("examples")
endif()

# hack-ish solution for ROOT
if(QUERYOSITY_ROOT)
  set(SETUP ${CMAKE_CURRENT_BINARY_DIR}/setup.sh)
  file(WRITE ${SETUP} "#!/bin/bash\n")
  file(APPEND ${SETUP} "# this is an auto-generated setup script\n" )
  file(APPEND ${SETUP} "export PYTHONPATH=\${PYTHONPATH}:${CMAKE_CURRENT_SOURCE_DIR}/\n")
  file(APPEND ${SETUP} "export QUERYOSITY_DIR=${CMAKE_CURRENT_SOURCE_DIR}\n")
  file(APPEND ${SETUP} "source ${CMAKE_CURRENT_BINARY_DIR}/extensions/setup.sh")
endif()