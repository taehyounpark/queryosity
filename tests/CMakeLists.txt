project(
  analogical_tests
  LANGUAGES CXX
)

find_package(analogical REQUIRED)

file(GLOB ANALOGICAL_PLUGINS_HEADERS plugins/*.h)
add_library(
  analogical_tests
  SHARED
  ${ANALOGICAL_PLUGINS_HEADERS}
)
set_target_properties(analogical_tests PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(
  analogical_tests
  analogical::analogical
)

find_package(Threads REQUIRED)

add_executable(test-selection ./test-selection.cxx)
target_compile_features(test-selection PUBLIC cxx_std_17)
target_link_libraries(test-selection analogical_tests pthread)

add_executable(test-multithread ./test-multithread.cxx)
target_compile_features(test-multithread PUBLIC cxx_std_17)
target_link_libraries(test-multithread analogical_tests pthread)