cmake_minimum_required(VERSION 2.8.11)

set(projname duktape_examples)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/duktape/src)

set(examples bind_class)

foreach(example ${examples})
    add_executable(${example} ${example})
    target_link_libraries(${example} duktape)
    set_property(TARGET ${example} PROPERTY CXX_STANDARD 14)
endforeach()
