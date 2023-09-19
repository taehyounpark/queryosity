project(analogical-tests)

add_subdirectory("plugins/")

add_executable(test-basic_selection ./test-basic_selection.cxx)
target_compile_features(test-basic_selection PUBLIC cxx_std_17)
target_include_directories(test-basic_selection PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/)
target_link_libraries(test-basic_selection analogical analogical_plugins)

add_executable(test-multithreading_consistency ./test-multithreading_consistency.cxx)
target_compile_features(test-multithreading_consistency PUBLIC cxx_std_17)
target_include_directories(test-multithreading_consistency PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/)
target_link_libraries(test-multithreading_consistency analogical analogical_plugins)