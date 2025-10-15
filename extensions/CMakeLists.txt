# collect all headers
file(GLOB QUERYOSITY_EXT_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/queryosity/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/queryosity/*/*.hpp
)

# nlohmann::json
if (QUERYOSITY_JSON)
  include(FetchContent)
  FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
  FetchContent_MakeAvailable(json)
else()
  list(FILTER QUERYOSITY_EXT_HEADERS EXCLUDE REGEX ".*/queryosity/nlohmann/.*\\.hpp$")
endif()

# rapidcsv
if (QUERYOSITY_CSV)
FetchContent_Declare(csv   
  GIT_REPOSITORY https://github.com/d99kris/rapidcsv.git
  GIT_TAG        7e87d8c
)
FetchContent_MakeAvailable(csv)
else()
  list(FILTER QUERYOSITY_EXT_HEADERS EXCLUDE REGEX ".*/queryosity/rapidcsv/.*\\.hpp$")
endif()

# boost::histogram
if (QUERYOSITY_BOOST)
  find_package( Boost CONFIG REQUIRED )
else()
  list(FILTER QUERYOSITY_EXT_HEADERS EXCLUDE REGEX ".*/queryosity/boost/.*\\.hpp$")
endif()

# ROOT
if (QUERYOSITY_ROOT)
  find_package(ROOT 6.36.00 QUIET COMPONENTS Core Hist RIO Tree ROOTDataFrame ROOTVecOps)
  find_package(BootstrapGenerator QUIET)
  if(NOT ROOT_FOUND)
      list(FILTER QUERYOSITY_EXT_HEADERS EXCLUDE REGEX ".*/queryosity/ROOT/.*\\.hpp$")
  endif()
  if(NOT BootstrapGenerator_FOUND)
      list(FILTER QUERYOSITY_EXT_HEADERS EXCLUDE REGEX ".*/queryosity/ROOT/.*_with_toys\\.hpp$")
  endif()
else()
  list(FILTER QUERYOSITY_EXT_HEADERS EXCLUDE REGEX ".*/queryosity/ROOT/.*\\.hpp$")
  list(FILTER QUERYOSITY_EXT_HEADERS EXCLUDE REGEX ".*/queryosity/ROOT/.*_with_toys\\.hpp$")
endif()

add_library(
  queryosity_extensions
  SHARED
  ${QUERYOSITY_EXT_HEADERS}
)
add_library(queryosity::extensions ALIAS queryosity_extensions)
set_target_properties(queryosity_extensions PROPERTIES EXPORT_NAME queryosity_extensions)
set_target_properties(queryosity_extensions PROPERTIES LINKER_LANGUAGE CXX)

target_compile_options(
  queryosity_extensions PUBLIC 
  -Wall -Wextra
  $<$<CONFIG:Release>:-O3> 
  $<$<CONFIG:RelWithDebInfo>:-O3> 
  $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-ggdb3> 
)

if(TARGET nlohmann_json)
  get_target_property(JSON_INCLUDE_DIRS nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
  target_link_libraries(queryosity_extensions PUBLIC nlohmann_json::nlohmann_json)
endif()

if(TARGET rapidcsv)
  get_target_property(CSV_INCLUDE_DIRS rapidcsv INTERFACE_INCLUDE_DIRECTORIES)
  target_link_libraries(queryosity_extensions PUBLIC rapidcsv)
endif()

if (BootstrapGenerator_FOUND)
  target_link_libraries(queryosity_extensions PUBLIC BootstrapGenerator)
endif()

target_link_libraries(
  queryosity_extensions
  INTERFACE queryosity::queryosity
  PUBLIC ${Boost_LIBRARIES} ${ROOT_LIBRARIES}
)

target_include_directories(queryosity_extensions PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${JSON_INCLUDE_DIRS}
  ${CSV_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${ROOT_INCLUDE_DIRS}
  ${BootstrapGenerator_INCLUDE_DIRS}
)

if (ROOT_FOUND)
  # ROOT Dictionary
  set(ROOT_DICTIONARY_HEADERS ${PROJECT_SOURCE_DIR}/include/queryosity.hpp)
  set(ROOT_DICTIONARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/queryosity/ROOT/LinkDef.h)

  # Start with an empty list
  set(ROOT_DICTIONARY_OPTIONS)

  # Append include directories
  foreach(dir ${Boost_INCLUDE_DIRS})
      list(APPEND ROOT_DICTIONARY_OPTIONS "-I${dir}")
  endforeach()

  foreach(dir ${QUERYOSITY_INCLUDE_DIRS})
      list(APPEND ROOT_DICTIONARY_OPTIONS "-I${dir}")
  endforeach()

  foreach(dir ${ROOT_INCLUDE_DIRS})
      list(APPEND ROOT_DICTIONARY_OPTIONS "-I${dir}")
  endforeach()

  # Optionally append BootstrapGenerator stuff
  if(BootstrapGenerator_FOUND)
      foreach(dir ${BootstrapGenerator_INCLUDE_DIRS})
          list(APPEND ROOT_DICTIONARY_OPTIONS "-I${dir}")
      endforeach()
      list(APPEND ROOT_DICTIONARY_OPTIONS "-DBOOTSTRAP_HISTOGRAM")
  endif()

  # Generate the dictionary
  ROOT_GENERATE_DICTIONARY(G__queryosity_dict
      ${ROOT_DICTIONARY_HEADERS}
      LINKDEF ${ROOT_DICTIONARY_SOURCES}
      OPTIONS ${ROOT_DICTIONARY_OPTIONS}
  )

  # Create the dictionary shared library
  add_library(queryosity_dict SHARED ${CMAKE_CURRENT_BINARY_DIR}/G__queryosity_dict.cxx)

  # Link to ROOT libraries and your backend
  # Start with libraries that are always linked
  set(QUERYOSITY_DICT_LIBS
      queryosity::extensions
      nlohmann_json::nlohmann_json
      rapidcsv
      ${Boost_LIBRARIES}
  )
  if(TARGET ROOT::Core)  # or use ROOT_FOUND if you have that
      list(APPEND QUERYOSITY_DICT_LIBS
          ROOT::Core
          ROOT::RIO
          ROOT::Hist
          ROOT::Tree
          ROOT::TreePlayer
          ROOT::Imt
          ROOT::ROOTVecOps
          ROOT::ROOTDataFrame
          ROOT::Physics
      )
  endif()
  if(TARGET BootstrapGenerator)
      list(APPEND QUERYOSITY_DICT_LIBS BootstrapGenerator)
  endif()
  target_link_libraries(queryosity_dict
    INTERFACE queryosity::queryosity
    PUBLIC ${QUERYOSITY_DICT_LIBS}
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
  file(APPEND ${SETUP} "export QUERYOSITY_EXTENSIONS_DIR=${CMAKE_CURRENT_SOURCE_DIR}\n")
endif()