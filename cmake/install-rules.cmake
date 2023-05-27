include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

install(
  DIRECTORY "${PROJECT_SOURCE_DIR}/ana/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT ana_Development
)

install(
  TARGETS ana
  EXPORT anaTargets
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
  anaConfigVersion.cmake
  COMPATIBILITY SameMajorVersion
  ARCH_INDEPENDENT
)

set(
  ana_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/ana"
  CACHE STRING "CMake package config location relative to the install prefix"
)

mark_as_advanced(ana_INSTALL_CMAKEDIR)

install(
  FILES
  "${PROJECT_SOURCE_DIR}/cmake/anaConfig.cmake"
  "${PROJECT_BINARY_DIR}/anaConfigVersion.cmake"
  DESTINATION "${ana_INSTALL_CMAKEDIR}"
  COMPONENT ana_Development
)

install(
  EXPORT anaTargets
  NAMESPACE ana::
  DESTINATION "${ana_INSTALL_CMAKEDIR}"
  COMPONENT ana_Development
)

# if(PROJECT_IS_TOP_LEVEL)
#   include(CPack)
# endif()