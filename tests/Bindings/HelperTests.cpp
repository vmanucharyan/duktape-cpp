#include <catch/catch.hpp>

#include <string>
#include <vector>

#include <Engine/Common/Utils/Helpers.h>

using namespace engine;

TEST_CASE("helper tests", "[duktape]") {
    SECTION("splitNamespaces") {
        std::string input("engine::duk::SomeClass");
        std::vector<std::string> expected { "engine", "duk", "SomeClass" };
        std::vector<std::string> actual = splitNamespaces(input);
        REQUIRE(expected == actual);

        SECTION("without namespace") {
            std::string input("SomeClass");
            std::vector<std::string> expected { "SomeClass" };
            std::vector<std::string> actual = splitNamespaces(input);
            REQUIRE(expected == actual);
        }
    }
}
