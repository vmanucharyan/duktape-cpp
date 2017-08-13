cmake_minimum_required(VERSION 2.8.11)

set(projname duktape_cpp)

file(GLOB_RECURSE source_files "src/*.cpp")
file(GLOB_RECURSE header_files "src/*.h")
file(GLOB_RECURSE inl_files "src/*.inl")

include_directories(src)
include_directories(dependencies/duktape/src)

add_library(duktape_cpp ${source_files} ${header_files} ${inl_files})

add_subdirectory(dependencies/duktape)

add_subdirectory(tests)

set_property(TARGET ${projname} PROPERTY CXX_STANDARD 14)