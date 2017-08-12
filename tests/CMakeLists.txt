cmake_minimum_required(VERSION 2.8.11)

set(projname engine_duktape_tests)

file(GLOB_RECURSE source_files "*.cpp")
file(GLOB_RECURSE header_files "*.h")

set(dependencies
    math_lib
    engine_main
    engine_duktape
)

add_executable(${projname} ${source_files} ${header_files})
add_test(${projname} ${projname})

# duktape
include_directories(${duktape_SOURCE_DIR})
target_link_libraries(${TARGET} duktape)

# catch
# import_catch(${projname})

foreach(dep ${dependencies})
    get_property(dep_includes TARGET ${dep} PROPERTY INCLUDE_DIRECTORIES)
    include_directories(${dep_includes})
    target_link_libraries(${projname} ${dep} ${${dep}_LIB_DEPENDS})
endforeach()

if (${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
    set_property(TARGET ${projname} PROPERTY FOLDER engine/extensions/duktape)
endif()

create_clang_complete(${projname})

set_property(TARGET ${projname} PROPERTY CXX_STANDARD 14)
