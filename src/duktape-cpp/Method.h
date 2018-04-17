#pragma once

#include <functional>
#include <unordered_map>

#include <duktape.h>

#include "./Utils/Helpers.h"

#include "Context.h"

#include "Type.h"

namespace duk { namespace details {

template <class A, int Index, bool IsPrimitive>
struct ArgGetter {
    static A get(duk::Context &d);
};

template <class A, int Index>
struct ArgGetter<A, Index, true> {
    static ClearType<A> get(duk::Context &d) {
        ClearType<A> instance;
        Type<ClearType<A>>::get(d, instance, Index);
        return instance;
    }
};

// object passed by value is copied
template <class A, int Index>
struct ArgGetter<A, Index, false> {
    static ClearType<A> get(duk::Context &d) {
        return ArgGetter<A, Index, true>::get(d);
    }
};

// reference to primitive type
template <class A, int Index>
struct ArgGetter<A const &, Index, true> {
    static A get(duk::Context &d) {
        // primitive types are copied from context
        return ArgGetter<ClearType<A>, Index, true>::get(d);
    }
};

// reference to object
template <class A, int Index>
struct ArgGetter<A&, Index, false> {
    static A& get(duk::Context &d) {
        if (!duk_has_prop_string(d, Index, "\xff" "obj_ptr")) {
            duk_error(d, DUK_ERR_TYPE_ERROR, "Expected reference to object, but `obj_ptr` not defined");
        }
        duk_get_prop_string(d, Index, "\xff" "obj_ptr");
        A *obj = reinterpret_cast<A*>(duk_get_pointer(d, -1));
        duk_pop(d);
        return *obj;
    }
};

template <class A, int Index>
struct ArgGetter<A const &, Index, false> {
    static A const & get(duk::Context &d) {
        return ArgGetter<A&, Index, false>::get(d);
    }
};

/**
 * Method dispatcher used to call native method and push result to duktape stack
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
        return func(obj, ArgGetter<A, I, Type<ClearType<A>>::isPrimitive()>::get(d)...);
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
        func(obj, ArgGetter<A, I, Type<ClearType<A>>::isPrimitive()>::get(d)...);
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
 * Method dispatcher used to call static native method and push result to duktape stack
 */
template <class R, class ... A>
struct StaticMethodDispatcher {
    duk_ret_t dispatch(std::function<R(A...)> const &func, duk::Context &d) {
        R res = call(func, d, std::index_sequence_for<A...>{});
        Type<ClearType<R>>::push(d, std::move(res));
        return 1;
    }

    template<std::size_t ... I>
    R call(std::function<R(A...)> const &func, duk::Context &d, std::index_sequence<I...>) {
        return func(ArgGetter<A, I, Type<ClearType<A>>::isPrimitive()>::get(d)...);
    }
};

template <class ... A>
struct StaticMethodDispatcher<void, A...> {
    duk_ret_t dispatch(std::function<void(A...)> const &func, duk::Context &d) {
        call(func, d, std::index_sequence_for<A...>{});
        return 0;
    }

    template<std::size_t ...I>
    void call(std::function<void(A...)> const &func, duk::Context &d, std::index_sequence<I...>) {
        func(ArgGetter<A, I, Type<ClearType<A>>::isPrimitive()>::get(d)...);
    }
};

template <>
struct StaticMethodDispatcher<void> {
    duk_ret_t dispatch(std::function<void()> const &func, duk::Context &d) {
        func();
        return 0;
    }
};

template <class R>
struct StaticMethodDispatcher<R> {
    duk_ret_t dispatch(std::function<R()> const &func, duk::Context &d) {
        R res = func();
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


/**
 * Push static method into duktape stack
 * Stores pointer to method as hidden `method_ptr` field
 *
 * @param duk_context pointer to duktape context
 * @param method pointer to method
 * @return index of function in duktape stack
 */
template <class R, class ... A>
struct StaticMethod {
    typedef std::function<R(A...)> MethodPointer;

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

        // Get pointer to method holder
        duk_push_current_function(d);
        duk_get_prop_string(d, -1, "\xff" "method_ptr");
        MethodPointer * mh = reinterpret_cast<MethodPointer*>(duk_get_pointer(d, -1));
        duk_pop_2(d);

        // Use method dispatcher to call method
        StaticMethodDispatcher<R, A...> m;
        return m.dispatch(*mh, *dd);
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

template <class R, class ... A>
void PushMethod(duk::Context &d, R (*method)(A...)) {
    StaticMethod<R, A...>::pushMethod(d, method);
}

}}
