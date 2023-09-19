
project(analogical_plugins)

file(GLOB ANALOGICAL_PLUGINS_Headers ./*.h)
add_library( analogical_plugins SHARED ${ANALOGICAL_PLUGINS_Headers} )

target_include_directories(analogical_plugins
                           PUBLIC
                           "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
                           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

set_target_properties(analogical_plugins PROPERTIES LINKER_LANGUAGE CXX)
target_compile_features(analogical_plugins PUBLIC cxx_std_17)

target_link_libraries(analogical_plugins analogical)
