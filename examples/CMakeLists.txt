find_package(Matplot++ REQUIRED)
find_package(Threads REQUIRED)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

add_executable(example-histogram ./example-histogram.cxx)
target_compile_features(example-histogram PUBLIC cxx_std_17)
target_link_libraries(example-histogram queryosity::plugins Matplot++::matplot pthread)
add_test(NAME example-histogram COMMAND example-histogram)