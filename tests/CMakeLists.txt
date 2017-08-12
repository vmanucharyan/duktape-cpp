cmake_minimum_required(VERSION 2.8.11)

set(projname duktape_plus_tests)

set(header_files Bindings/TestTypes.h)

set(source_files main.cpp
    Bindings/ConstructorTests.cpp
    Bindings/ContextTests.cpp
    Bindings/FunctionTests.cpp
    Bindings/HelperTests.cpp
    Bindings/MethodTests.cpp
    Bindings/PushObjectInspectorTests.cpp
    Bindings/SharedPtrTests.cpp
    Bindings/STLTypesTests.cpp
)

add_executable(${projname} ${source_files} ${header_files})
add_test(${projname} ${projname})

# duktape
include_directories(${duktape_SOURCE_DIR}/src)
target_link_libraries(${projname} duktape)
target_link_libraries(${projname} duktape_plus)

# catch
include_directories(${CMAKE_SOURCE_DIR}/dependencies/catch)

set_property(TARGET ${projname} PROPERTY CXX_STANDARD 14)
