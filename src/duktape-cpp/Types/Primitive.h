#include "../Type.h"

namespace duk {

template <>
struct Type<int> {
    static void push(duk::Context &d, int val) {
        duk_push_int(d, val);
    }

    static void get(duk::Context &d, int &val, int index) {
        val = duk_require_int(d, index);
    }

    static constexpr bool isPrimitive() { return true; };
};
    
template <>
struct Type<unsigned int> {
    static void push(duk::Context &d, unsigned int val) {
        duk_push_uint(d, val);
    }

    static void get(duk::Context &d, unsigned int &val, int index) {
        val = duk_require_uint(d, index);
    }

    static constexpr bool isPrimitive() { return true; };
};

template <>
struct Type<float> {
    static void push(duk::Context &d, float val) {
        duk_push_number(d, val);
    }

    static void get(duk::Context &d, float &val, int index) {
        val = float(duk_require_number(d, index));
    }

    static constexpr bool isPrimitive() { return true; };
};

template <>
struct Type<double> {
    static void push(duk::Context &d, double val) {
        duk_push_number(d, val);
    }

    static void get(duk::Context &d, double &val, int index) {
        val = double(duk_require_number(d, index));
    }

    static constexpr bool isPrimitive() { return true; };
};

template <>
struct Type<bool> {
    static void push(duk::Context &d, bool val) {
        duk_push_boolean(d, val);
    }

    static void get(duk::Context &d, bool &val, int index) {
        val = bool(duk_require_boolean(d, index));
    }

    static constexpr bool isPrimitive() { return true; };
};

}
