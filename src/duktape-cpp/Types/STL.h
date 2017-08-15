#pragma once

#include <string>

#include "../Context.h"
#include "../Type.h"

namespace duk {

template <>
struct Type<std::string> {
    static void push(duk::Context &d, std::string const &value) {
        duk_push_string(d, value.c_str());
    }

    static void get(duk::Context &d, std::string &value, int index) {
        const char *cstr = duk_get_string(d, index);
        value = std::string(cstr);
    }

    static constexpr bool isPrimitive() { return true; };
};

template <class T>
struct Type<std::vector<T>> {
    static void push(duk::Context &d, std::vector<T> const &value) {
        duk_push_array(d);
        for (int i = 0; i < value.size(); ++i) {
            Type<T>::push(d, value[i]);
            duk_put_prop_index(d, -2, i);
        }
    }

    static void get(duk::Context &d, std::vector<T> &value, int index) {
        duk_enum(d, index, DUK_ENUM_ARRAY_INDICES_ONLY);

        while (duk_next(d, -1, 1)) {
            T val;
            Type<T>::get(d, val, -1);
            value.push_back(val);
            duk_pop_2(d);
        }

        duk_pop(d);
    }

    static constexpr bool isPrimitive() { return true; };
};

}
