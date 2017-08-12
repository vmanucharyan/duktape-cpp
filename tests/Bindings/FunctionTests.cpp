#include <catch/catch.hpp>

#include <Engine/Duktape/Bindings/Prelude.h>

using namespace engine;

TEST_CASE("Functions tests", "[duktape]") {
    duk::Context d;

    SECTION("get") {
        SECTION("should return std function that calls js function") {
            // arrange
            std::function<int()> f;
            const char * script = "var f = function () { return 321; }; f";

            // act
            d.evalString(f, script);

            // assert
            REQUIRE(f() == 321);

            SECTION("does not pollute stash") {
                REQUIRE(duk_get_top(d) == 0);
            }
        }

        SECTION("should call the right function") {
            std::function<bool(int)> f;
            const char script[] =
                "function f1(a) { return  a == 1; }\n"
                "function f2(a) { return a == 2; }\n"
                "function f3(a) { return a == 3; }\n"
                "f2";

            d.evalString(f, script);

            REQUIRE(f(2) == true);
        }

        SECTION("should handle functions with parameters") {
            // arrange
            std::function<Vec3(Vec3 const &v, float k)> f;
            const char script[] =
                "var f = function(v, k) { return { x: v.x * k, y: v.y * k, z: v.z * k }; }; f";
            Vec3 paramV(1, 2, 3);
            float paramK = 3.0f;
            Vec3 expectedRes = paramV * paramK;

            // act
            d.evalString(f, script);
            Vec3 actualRes = f(paramV, paramK);

            // assert
            REQUIRE(actualRes == expectedRes);

            SECTION("does not pollute stash") {
                REQUIRE(duk_get_top(d) == 0);
            }
        }
    }
}
