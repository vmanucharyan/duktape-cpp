cmake_minimum_required(VERSION 2.8.11)

project(.)

add_definitions(-DDUK_OPT_CPP_EXCEPTIONS)

set_source_files_properties(duktape.c PROPERTIES LANGUAGE CXX)

add_library(. src/duktape.c src/duktape.h src/duk_config.h)
