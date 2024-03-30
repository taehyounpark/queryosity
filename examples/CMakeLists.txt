if (QUERYOSITY_EXAMPLES)

  add_executable(example-hello_world ./example-hello_world.cxx)
  target_compile_features(example-hello_world PUBLIC cxx_std_17)
  target_link_libraries(example-hello_world queryosity::extensions pthread ${Boost_LIBRARIES})
  add_test(NAME example-hello_world COMMAND example-hello_world)

  add_executable(example-multiple_datasets ./example-multiple_datasets.cxx)
  target_compile_features(example-multiple_datasets PUBLIC cxx_std_17)
  target_link_libraries(example-multiple_datasets queryosity::extensions pthread ${Boost_LIBRARIES})
  add_test(NAME example-multiple_datasets COMMAND example-multiple_datasets)

endif()
