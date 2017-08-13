cmake_minimum_required(VERSION 2.8.11)

project(duktape)

add_definitions(-DDUK_OPT_CPP_EXCEPTIONS)

set_source_files_properties(src/duktape.c PROPERTIES LANGUAGE CXX)

add_library(duktape src/duktape.c src/duktape.h src/duk_config.h)
