#include <catch/catch.hpp>

#include <duktape-cpp/DuktapeCpp.h>

#include "TestTypes.h"

TEST_CASE("Tuple", "[duktape]") {
    duk::Context d;

    SECTION("should be able to push and pop") {
        SECTION("1-element tuple") {
            // arrange
            std::tuple<int> pushedVal = std::make_tuple(123);
            std::tuple<int> poppedVal;

            // act
            duk::Type<std::tuple<int>>::push(d, pushedVal);
            duk::Type<std::tuple<int>>::get(d, poppedVal, -1);
            duk_pop(d);

            // assert
            REQUIRE(pushedVal == poppedVal);
            REQUIRE(duk_get_top(d) == 0);
        }

        SECTION("n-element tuple") {
            // arrange
            std::tuple<int, std::string, Vec2> pushedVal = std::make_tuple(123, "321", Vec2(12.0f, 21.0f));
            std::tuple<int, std::string, Vec2> poppedVal;

            // act
            duk::Type<std::tuple<int, std::string, Vec2>>::push(d, pushedVal);
            duk::Type<std::tuple<int, std::string, Vec2>>::get(d, poppedVal, -1);
            duk_pop(d);

            // assert
            REQUIRE(pushedVal == poppedVal);
            REQUIRE(duk_get_top(d) == 0);
        }
    }
}