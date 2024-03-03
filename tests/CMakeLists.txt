if (ANALOGICAL_TESTS)

  find_package(Threads REQUIRED)

  find_package(Boost REQUIRED)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})

  # add_executable(test-definition ./test-definition.cxx)
  # target_compile_features(test-definition PUBLIC cxx_std_17)
  # target_link_libraries(test-definition queryosity::plugins pthread ${Boost_LIBRARIES})
  # add_test(NAME test-definition COMMAND test-definition)

  add_executable(test-selection ./test-selection.cxx)
  target_compile_features(test-selection PUBLIC cxx_std_17)
  target_link_libraries(test-selection queryosity::plugins pthread)
  add_test(NAME test-selection COMMAND test-selection)

  add_executable(test-multithreading ./test-multithreading.cxx)
  target_compile_features(test-multithreading PUBLIC cxx_std_17)
  target_link_libraries(test-multithreading queryosity::plugins pthread)
  add_test(NAME test-multithreading COMMAND test-multithreading)

  add_executable(test-variation ./test-variation.cxx)
  target_compile_features(test-variation PUBLIC cxx_std_17)
  target_link_libraries(test-variation queryosity::plugins pthread ${Boost_LIBRARIES})
  add_test(NAME test-variation COMMAND test-variation)
endif()
