cmake_minimum_required(VERSION 3.12)

project(ana 
  VERSION 0.1
  DESCRIPTION "Coherent data dataflow interface in C++"
  HOMEPAGE_URL "https://github.com/taehyounpark/ana.git"
  LANGUAGES CXX
  )

add_library(ana INTERFACE)

set_property(
  TARGET ana PROPERTY
  EXPORT_NAME ana
)

target_include_directories(ana INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>")
target_compile_features(ana INTERFACE cxx_std_17)