#pragma once

#include <functional>
#include <unordered_map>

#include <duktape.h>

#include "Duktape/Utils/Helpers.h"

#include "Context.h"

#include "Type.h"

namespace engine { namespace duk { namespace details {

template <class A, int Index>
ClearType<A> GetArg(duk::Context &d) {
    ClearType<A> instance;
    Type<ClearType<A>>::get(d, instance, Index);
    return instance;
}

/**
 * Method dispatcher used to call native method and push result to duktape stack
 *
 * http://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
 */
template <class C, class R, class ... A>
struct MethodDispatcher {
    duk_ret_t dispatch(std::function<R(C*, A...)> const &func, C* obj, duk::Context &d) {
        R res = call(func, obj, d, std::index_sequence_for<A...>{});
        Type<ClearType<R>>::push(d, std::move(res));
        return 1;
    }

    template<std::size_t ... I>
    R call(std::function<R(C*, A...)> const &func, C* obj, duk::Context &d, std::index_sequence<I...>) {
        return func(obj, GetArg<A, I>(d)...);
    }
};

template <class C, class ... A>
struct MethodDispatcher<C, void, A...> {
    duk_ret_t dispatch(std::function<void(C*, A...)> const &func, C* obj, duk::Context &d) {
        call(func, obj, d, std::index_sequence_for<A...>{});
        return 0;
    }

    template<std::size_t ...I>
    void call(std::function<void(C*, A...)> const &func, C* obj, duk::Context &d, std::index_sequence<I...>) {
        func(obj, GetArg<A, I>(d)...);
    }
};

template <class C>
struct MethodDispatcher<C, void> {
    duk_ret_t dispatch(std::function<void(C*)> const &func, C* obj, duk::Context &d) {
        func(obj);
        return 0;
    }
};

template <class C, class R>
struct MethodDispatcher<C, R> {
    duk_ret_t dispatch(std::function<R(C*)> const &func, C* obj, duk::Context &d) {
        R res = func(obj);
        Type<ClearType<R>>::push(d, res);
        return 1;
    }
};

/**
 * Push method into duktape stack
 * Stores pointer to method as hidden `method_ptr` field
 *
 * @param duk_context pointer to duktape context
 * @param method pointer to method
 * @return index of function in duktape stack
 */
template <class C, class R, class ... A>
struct Method {
    typedef std::function<R(C*, A...)> MethodPointer;

    static int pushMethod(duk::Context &d, MethodPointer method) {
        // Store method pointer
        auto *mh = new MethodPointer(method);

        // Push function into stack
        auto fidx = duk_push_c_function(d, func, sizeof...(A));

        // Add hidden pointer to method holder
        duk_push_pointer(d, mh);
        duk_put_prop_string(d, fidx, "\xff" "method_ptr");

        duk_push_c_function(d, funcFinalizer, 1);
        duk_set_finalizer(d, fidx);

        return fidx;
    }

    /**
     * This function is an entry point for methods called from duktape.
     * It gets object and method pointers, reads parameters from duktape
     * stack and makes actual calls to methods.
     */
    static duk_ret_t func(duk_context *d) {
        // Get pointer to context
        duk_push_global_stash(d);
        duk_get_prop_string(d, -1, "self_ptr");
        Context * dd = reinterpret_cast<Context*>(duk_get_pointer(d, -1));
        duk_pop_2(d);

        // Get pointer to object
        duk_push_this(d);
        duk_get_prop_string(d, -1, "\xff" "obj_ptr");
        C * objPtr = reinterpret_cast<C*>(duk_get_pointer(d, -1));
        duk_pop_2(d);

        // Get pointer to method holder
        duk_push_current_function(d);
        duk_get_prop_string(d, -1, "\xff" "method_ptr");
        MethodPointer * mh = reinterpret_cast<MethodPointer*>(duk_get_pointer(d, -1));
        duk_pop_2(d);

        // Use method dispatcher to call method
        MethodDispatcher<C, R, A...> m;
        return m.dispatch(*mh, objPtr, *dd);
    }

    static duk_ret_t funcFinalizer(duk_context *d) {
        // object being finalized is at index 0
        duk_get_prop_string(d, 0, "\xff" "method_ptr");
        void * methodPtr = duk_get_pointer(d, -1);
        duk_pop(d);

        MethodPointer * mh = reinterpret_cast<MethodPointer*>(methodPtr);
        delete mh;

        return 0;
    }
};

template <class C, class R, class ... A>
void PushMethod(duk::Context &d, R (C::*method)(A...)) {
    Method<C, R, A...>::pushMethod(d, method);
}

template <class C, class R, class ... A>
void PushMethod(duk::Context &d, R (C::*method)(A...) const) {
    Method<C, R, A...>::pushMethod(d, method);
}

}}}
