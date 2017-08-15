#pragma once

#include <duktape.h>

#include "Context.h"

namespace duk {

class Context;

/**
 * Type that can be converted to/from javascript
 */
template <class T>
struct Type {
    /**
     * Push value to stack
     * @param d pointer to duktape context
     * @param val value
     */
    static void push(duk::Context &d, T const &val);

    /**
     * Get value of type T from stack at specified index
     * @param[in] d pointer to duktape context
     * @param[out] val value
     */
    static void get(duk::Context &d, T &val, int index);

    /**
     * Indicates if type is primitive
     */
    static constexpr bool isPrimitive() { return false; };
};

}
