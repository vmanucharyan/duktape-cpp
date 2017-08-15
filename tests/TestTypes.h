#pragma once

#include <duktape.h>
#include <duktape-cpp/Context.h>

struct Vec2 {
    Vec2() = default;

    Vec2(float x, float y) : x(x), y(y) {}

    bool operator==(const Vec2 &rhs) const {
        return fabsf(x - rhs.x) < 1e-6 &&
               fabsf(y - rhs.y) < 1e-6;
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

    static constexpr bool isPrimitive() { return true; }
};

}

struct Vec3 {
    Vec3() = default;

    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    bool operator==(const Vec3 &rhs) const {
        return x == rhs.x &&
               y == rhs.y &&
               z == rhs.z;
    }

    bool operator!=(const Vec3 &rhs) const {
        return !(rhs == *this);
    }

    float x {0.0f};
    float y {0.0f};
    float z {0.0f};
};

namespace duk {

template <>
struct Type<Vec3> {
    static void push(duk::Context &d, Vec3 const &val) {
        duk_push_object(d);

        duk_push_number(d, val.x);
        duk_put_prop_string(d, -2, "x");

        duk_push_number(d, val.y);
        duk_put_prop_string(d, -2, "y");

        duk_push_number(d, val.z);
        duk_put_prop_string(d, -2, "z");
    }

    static void get(duk::Context &d, Vec3 &val, int index) {
        duk_get_prop_string(d, index, "x");
        float x = float(duk_get_number(d, -1));
        duk_pop(d);

        duk_get_prop_string(d, index, "y");
        float y = float(duk_get_number(d, -1));
        duk_pop(d);

        duk_get_prop_string(d, index, "z");
        float z = float(duk_get_number(d, -1));
        duk_pop(d);

        val = Vec3(x, y, z);
    }

    static constexpr bool isPrimitive() { return true; }
};

}