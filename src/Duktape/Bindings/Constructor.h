#pragma once

#include <duktape.h>

namespace duk {

class Context;

namespace details {

/**
 * Constructor bindings
 * @tparam C class type
 * @tparam A argument types
 */
template <class C, class ... A>
struct Constructor {
    typedef std::shared_ptr<C> (*TFunc)(A...);

    /**
     * Push constructor to stack
     * @param d duktape context
     * @param constructor pointer to constructor function
     */
    static void push(duk::Context &d, TFunc constructor);

    /**
     * Entry point for constructor calls from javascript
     * @param d duktape context
     */
    static duk_ret_t func(duk_context *d);
};

/**
 * Unique pointer constructor bindings
 * @tparam C class type
 * @tparam A argument types
 */
template <class C, class ... A>
struct ConstructorUnique {
    typedef std::unique_ptr<C> (*TFunc)(A...);

    /**
     * Push constructor to stack
     * @param d duktape context
     * @param constructor pointer to constructor function
     */
    static void push(duk::Context &d, TFunc constructor);

    /**
     * Entry point for constructor calls from javascript
     * @param d duktape context
     */
    static duk_ret_t func(duk_context *d);
};

} // details

} // engine::duk
