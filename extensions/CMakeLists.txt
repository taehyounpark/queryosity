file(GLOB QUERYOSITY_EXT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/queryosity/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/queryosity/*/*.hpp)
add_library(
  queryosity_extensions
  SHARED
  ${QUERYOSITY_EXT_HEADERS}
)
add_library(queryosity::extensions ALIAS queryosity_extensions)
set_target_properties(queryosity_extensions PROPERTIES EXPORT_NAME queryosity_extensions)
set_target_properties(queryosity_extensions PROPERTIES LINKER_LANGUAGE CXX)

target_compile_features(queryosity_extensions PUBLIC "cxx_std_${QUERYOSITY_CXX_STANDARD}" )
target_compile_options(
  queryosity_extensions PUBLIC 
  -Wall -Wextra
  $<$<CONFIG:Release>:-O3> 
  $<$<CONFIG:RelWithDebInfo>:-O3> 
  $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-ggdb3> 
)

# boost::histogram
find_package(ROOT REQUIRED COMPONENTS Core Imt RIO Net Hist Graf Graf3d Gpad ROOTVecOps Tree TreePlayer Postscript Matrix Physics MathCore Thread MultiProc ROOTDataFrame )

# boost::histogram
find_package(Boost REQUIRED)

# nlohmann::json
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

FetchContent_Declare(csv   
  GIT_REPOSITORY https://github.com/d99kris/rapidcsv.git
  GIT_TAG        7e87d8c
)
FetchContent_MakeAvailable(csv)

target_include_directories(
  queryosity_extensions 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Boost_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS}
)
include(ExternalProject)

target_link_libraries(
  queryosity_extensions
  INTERFACE queryosity::queryosity
  PUBLIC nlohmann_json::nlohmann_json rapidcsv ROOT::Core ROOT::RIO ROOT::Hist ROOT::Tree ROOT::TreePlayer ROOT::Imt ROOT::ROOTVecOps ROOT::ROOTDataFrame ROOT::Physics ${Boost_LIBRARIES}
)