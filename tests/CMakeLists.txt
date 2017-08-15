cmake_minimum_required(VERSION 2.8.11)

set(projname duktape_cpp_tests)

set(header_files TestTypes.h)

set(source_files ./main.cpp
    ./ConstructorTests.cpp
    ./ContextTests.cpp
    ./FunctionTests.cpp
    ./HelperTests.cpp
    ./MethodTests.cpp
    ./PushObjectInspectorTests.cpp
    ./SharedPtrTests.cpp
    ./STLTypesTests.cpp
    ./TuplesTest.cpp
    ./PolymorphicTypesTests.cpp
)

add_executable(${projname} ${source_files} ${header_files})
add_test(${projname} ${projname})

# duktape
include_directories(${CMAKE_SOURCE_DIR}/dependencies/duktape)
target_link_libraries(${projname} duktape)

# catch
include_directories(${CMAKE_SOURCE_DIR}/dependencies/catch)

set_property(TARGET ${projname} PROPERTY CXX_STANDARD 14)
