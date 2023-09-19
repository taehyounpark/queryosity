project(
  analogical-tests
  LANGUAGES CXX
)
set(ANALOGICAL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include/)
file(GLOB ANALOGICAL_TESTS_Headers plugins/*.h)

add_library(
  analogical-tests
  SHARED
  ${ANALOGICAL_TESTS_Headers}
)

set_target_properties(analogical-tests PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(
  analogical-tests PUBLIC ${ANALOGICAL_INCLUDE_DIR}
)
target_link_libraries(
  analogical-tests
  analogical
)

find_package(Threads REQUIRED)

add_executable(test-selection ./test-selection.cxx)
target_compile_features(test-selection PUBLIC cxx_std_17)
target_include_directories(test-selection PUBLIC ${ANALOGICAL_INCLUDE_DIR})
target_link_libraries(test-selection analogical pthread)

add_executable(test-multithread ./test-multithread.cxx)
target_compile_features(test-multithread PUBLIC cxx_std_17)
target_include_directories(test-multithread PUBLIC ${ANALOGICAL_INCLUDE_DIR})
target_link_libraries(test-multithread analogical pthread)