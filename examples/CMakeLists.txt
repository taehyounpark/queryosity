if(QUERYOSITY_EXAMPLES)

  install(FILES ${CMAKE_SOURCE_DIR}/data/data.json
          DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

  install(FILES ${CMAKE_SOURCE_DIR}/data/data.csv
          DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

  add_executable(example-01 ./example-01.cxx)
  target_compile_features(example-01 PUBLIC cxx_std_17)
  target_link_libraries(example-01 queryosity::extensions pthread
                        ${Boost_LIBRARIES})
  add_test(NAME example-01 COMMAND example-01)

  add_executable(example-02 ./example-02.cxx)
  target_compile_features(example-02 PUBLIC cxx_std_17)
  target_link_libraries(example-02 queryosity::extensions pthread
                        ${Boost_LIBRARIES})
  add_test(NAME example-02 COMMAND example-02)

  add_executable(example-03 ./example-03.cxx)
  target_compile_features(example-03 PUBLIC cxx_std_17)
  target_link_libraries(example-03 queryosity::extensions pthread) 
  add_test(NAME example-03 COMMAND example-03)

  add_executable(example-04 ./example-04.cxx)
  target_compile_features(example-04 PUBLIC cxx_std_17)
  target_link_libraries(example-04 queryosity::extensions pthread) 
  add_test(NAME example-04 COMMAND example-04)

  add_executable(example-tree ./example-tree.cxx)
  target_compile_features(example-tree PUBLIC cxx_std_17)
  target_link_libraries(example-tree queryosity::extensions pthread) 
  add_test(NAME example-tree COMMAND example-tree)

endif()
