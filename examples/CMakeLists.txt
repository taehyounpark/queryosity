if (QUERYOSITY_EXAMPLES)

  add_executable(example-hello_world ./example-hello_world.cxx)
  target_compile_features(example-hello_world PUBLIC cxx_std_17)
  target_link_libraries(example-hello_world queryosity::extensions pthread ${Boost_LIBRARIES})
  add_test(NAME example-hello_world COMMAND example-hello_world)

endif()
