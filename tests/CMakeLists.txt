find_package(Threads REQUIRED)

add_executable(test-selection ./test-selection.cxx)
target_compile_features(test-selection PUBLIC cxx_std_17)
target_link_libraries(test-selection ana::plugins pthread)
add_test(NAME test-selection COMMAND test-selection)

add_executable(test-multithread ./test-multithread.cxx)
target_compile_features(test-multithread PUBLIC cxx_std_17)
target_link_libraries(test-multithread ana::plugins pthread)
add_test(NAME test-multithread COMMAND test-multithread)