file(GLOB QUERYOSITY_EXT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/queryosity/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/queryosity/*/*.hpp)
add_library(
  queryosity_backends
  SHARED
  ${QUERYOSITY_EXT_HEADERS}
)
add_library(queryosity::extensions ALIAS queryosity_backends)
set_target_properties(queryosity_backends PROPERTIES EXPORT_NAME queryosity_backends)
set_target_properties(queryosity_backends PROPERTIES LINKER_LANGUAGE CXX)

target_compile_options(
  queryosity_backends PUBLIC 
  -Wall -Wextra
  $<$<CONFIG:Release>:-O3> 
  $<$<CONFIG:RelWithDebInfo>:-O3> 
  $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-ggdb3> 
)

# ROOT
find_package( ROOT 6.36.00 REQUIRED COMPONENTS Core Hist RIO Tree ROOTDataFrame ROOTVecOps )

# boost::histogram
find_package( Boost CONFIG REQUIRED )

# find_package( BootstrapGenerator REQUIRED )

# nlohmann::json
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

FetchContent_Declare(csv   
  GIT_REPOSITORY https://github.com/d99kris/rapidcsv.git
  GIT_TAG        7e87d8c
)
FetchContent_MakeAvailable(csv)

get_target_property(CSV_INCLUDE_DIR rapidcsv INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(JSON_INCLUDE_DIR nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories(
  queryosity_backends 
  PUBLIC 
  ${PROJECT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CSV_INCLUDE_DIR}
  ${JSON_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${ROOT_INCLUDE_DIRS}
)
include(ExternalProject)

target_link_libraries(
  queryosity_backends
  INTERFACE queryosity::queryosity
  PUBLIC nlohmann_json::nlohmann_json rapidcsv ${Boost_LIBRARIES} ROOT::Core ROOT::Gpad ROOT::Graf3d ROOT::Graf ROOT::Hist ROOT::Imt ROOT::MathCore ROOT::Matrix ROOT::MultiProc ROOT::Net ROOT::Physics ROOT::Postscript ROOT::RIO ROOT::ROOTDataFrame ROOT::ROOTVecOps ROOT::Rint ROOT::Thread ROOT::TreePlayer ROOT::Tree
)

# ROOT Dictionary
set(DICTIONARY_HEADERS ${PROJECT_SOURCE_DIR}/include/queryosity.hpp)
set(DICTIONARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/queryosity/ROOT/LinkDef.h)

ROOT_GENERATE_DICTIONARY(G__queryosity_dict
    ${DICTIONARY_HEADERS}
    LINKDEF ${DICTIONARY_SOURCES}
    OPTIONS -I${PROJECT_SOURCE_DIR}/include
            -I${CMAKE_CURRENT_SOURCE_DIR}
            -I${Boost_INCLUDE_DIRS}
            -I${ROOT_INCLUDE_DIRS}
)

# Create the dictionary shared library
add_library(queryosity_dict SHARED ${CMAKE_CURRENT_BINARY_DIR}/G__queryosity_dict.cxx)

# Link to ROOT libraries and your backend
target_link_libraries(queryosity_dict
    PUBLIC nlohmann_json::nlohmann_json rapidcsv ROOT::Core ROOT::RIO ROOT::Hist ROOT::Tree ROOT::TreePlayer ROOT::Imt ROOT::ROOTVecOps ROOT::ROOTDataFrame ROOT::Physics ${Boost_LIBRARIES}
    PRIVATE queryosity_backends
)

# Install the dictionary .so, .pcm, and .rootmap
install(TARGETS queryosity_dict
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# setup script
set(SETUP ${CMAKE_CURRENT_BINARY_DIR}/setup.sh)
file(WRITE ${SETUP} "#!/bin/bash\n")
file(APPEND ${SETUP} "# this is an auto-generated setup script\n" )
file(APPEND ${SETUP} "export LD_LIBRARY_PATH=\${LD_LIBRARY_PATH}:${CMAKE_INSTALL_PREFIX}/lib\n")
file(APPEND ${SETUP} "export LD_LIBRARY_PATH=\${LD_LIBRARY_PATH}:${CMAKE_CURRENT_BINARY_DIR}/\n")
file(APPEND ${SETUP} "export PATH=\${PATH}:${CMAKE_CURRENT_SOURCE_DIR}/python\n")
file(APPEND ${SETUP} "export QUERYOSITY_BACKENDS_DIR=${CMAKE_CURRENT_SOURCE_DIR}\n")