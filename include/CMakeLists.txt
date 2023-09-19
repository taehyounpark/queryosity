add_library(analogical INTERFACE)

set_property(
  TARGET analogical PROPERTY
  EXPORT_NAME ana
)

target_include_directories(analogical
                           INTERFACE
                           "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include"
                           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_compile_features(analogical INTERFACE cxx_std_17)
add_compile_options(
  -Wall
  -Werror
  -Wno-error=maybe-uninitialized
  $<$<CONFIG:RELEASE>:-O3>
  $<$<CONFIG:DEBUG>:-O0>
  $<$<CONFIG:DEBUG>:-ggdb3>
)