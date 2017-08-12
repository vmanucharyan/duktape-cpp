#include <catch/catch.hpp>

#include <iostream>

#include <duktape.h>

#include <Engine/Duktape/Bindings/Context.inl>
#include <Engine/Duktape/Bindings/Types/Math.h>

using namespace engine;

TEST_CASE("Math types bindings tests", "[duktape]") {
    duk::Context ctx;
    duk_context *d = ctx.ptr();

    SECTION("should be able to push and get Vec3") {
        Vec3 pushed(15.0f, 0.5f, 5.5f);
        duk::Type<Vec3>::push(d, pushed);

        Vec3 popped;
        duk::Type<Vec3>::get(d, popped, -1);

        REQUIRE(pushed == popped);
    }

    SECTION("should be able to push and get Vec2") {
        Vec2 pushed(15.0f, 0.5f);
        duk::Type<Vec2>::push(d, pushed);

        Vec2 popped;
        duk::Type<Vec2>::get(d, popped, -1);

        REQUIRE(pushed == popped);
    }

    SECTION("should be able to push and get Quat") {
        Quat pushed(15.0f, 0.5f, 12.0f, 0.0f);
        duk::Type<Quat>::push(d, pushed);

        Quat popped;
        duk::Type<Quat>::get(d, popped, -1);

        REQUIRE(pushed == popped);
    }
}
