#include <catch/catch.hpp>

#include <duktape-cpp/DuktapeCpp.h>

using namespace duk;

using namespace duk;

struct Vec2 {
    Vec2() = default;

    Vec2(float x, float y) : x(x), y(y) {}

    bool operator==(const Vec2 &rhs) const {
        return x == rhs.x &&
               y == rhs.y;
    }

    bool operator!=(const Vec2 &rhs) const {
        return !(rhs == *this);
    }

    float x {0.0f};
    float y {0.0f};
};

namespace duk {

template <>
struct Type<Vec2> {
    static void push(duk::Context &d, Vec2 const &val) {
        duk_push_object(d);

        duk_push_number(d, val.x);
        duk_put_prop_string(d, -2, "x");

        duk_push_number(d, val.y);
        duk_put_prop_string(d, -2, "y");
    }

    static void get(duk::Context &d, Vec2 &val, int index) {
        duk_get_prop_string(d, index, "x");
        float x = float(duk_get_number(d, -1));
        duk_pop(d);

        duk_get_prop_string(d, index, "y");
        float y = float(duk_get_number(d, -1));
        duk_pop(d);

        val = Vec2(x, y);
    }
};

}

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
            std::function<Vec2(Vec2 const &v, float k)> f;
            const char script[] = "var f = function(v, k) { return { x: v.x * k, y: v.y * k } }; f";
            Vec2 paramV(1.0f, 2.0f);
            float paramK = 3.0f;
            Vec2 expectedRes = Vec2(3.0f, 6.0f);

            // act
            d.evalString(f, script);
            Vec2 actualRes = f(paramV, paramK);

            // assert
            REQUIRE(actualRes == expectedRes);

            SECTION("does not pollute stash") {
                REQUIRE(duk_get_top(d) == 0);
            }
        }
    }
}
