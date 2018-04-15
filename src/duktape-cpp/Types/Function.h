#pragma once

#include <functional>
#include <cassert>
#include <cstdio>

#include "../Context.h"
#include "../Type.h"
#include "../Exceptions.h"

namespace duk { namespace details {

template <class R>
struct JSFunctionReturnVal {
    static R get(Context &d, int numPops) {
        R res;
        Type<R>::get(d, res, -1);
        duk_pop(d);
        duk_pop_n(d, numPops);
        return std::move(res);
    }
};

template <>
struct JSFunctionReturnVal<void> {
    static void get(Context &d, int numPops) {
        duk_pop(d);
        duk_pop_n(d, numPops);
    }
};

}

template <class R, class ... A>
class JSFunction {
public:
    JSFunction(duk_context *d, int stashIndex)
        : _d(d), _refKey(stashIndex) { }

    ~JSFunction() {
        if (_d) {
            Context &ctx = Context::GetSelfFromContext(_d);
            ctx.unstashRef(_refKey);
        }
    }

    JSFunction(JSFunction const &that)
        : _d(that._d), _refKey(that._refKey) {
        JSFunction &thatt = const_cast<JSFunction&>(that);
        thatt._d = nullptr;
        thatt._refKey = -1;
    }

    JSFunction & operator= (JSFunction const &that) {
        if (this == &that) {
            return *this;
        }

        _d = that._d;
        _refKey = that._refKey;

        JSFunction &thatt = const_cast<JSFunction&>(that);
        thatt._d = nullptr;
        thatt._refKey = -1;

        return *this;
    }

    JSFunction(JSFunction &&that)
        : _d(that._d), _refKey(that._refKey) {
        that._d = nullptr;
        that._refKey = -1;
    }

    JSFunction & operator= (JSFunction &&that) {
        if (this == &that) {
            return *this;
        }

        _d = that._d;
        _refKey = that._refKey;

        that._d = nullptr;
        that._refKey == -1;

        return *this;
    }

    R operator () (A&& ... args) const {
        return call(std::forward<A>(args)...);
    }

    R call(A&& ... args) const {
        assert(_d);
        assert(_refKey >= 0);

        Context &d = Context::GetSelfFromContext(_d);
        d.getRef(_refKey); 

        pushArgs(d, std::forward<A>(args)...);
        duk_int_t callRes = duk_pcall(d, sizeof...(args));
        if (callRes != DUK_EXEC_SUCCESS) {
            duk_get_prop_string(d, -1, "stack");
            std::string stack = duk_get_string(d, -1);
            duk_pop(d);
            throw ScriptEvaluationExcepton(std::string(duk_safe_to_string(d, -1)) + "\n" + stack);
        }
        return details::JSFunctionReturnVal<R>::get(d, 0);
    }

private:
    duk_context *_d;
    int _refKey;

    void pushArgs(duk::Context &d) const {
        // Do nothing
    }

    template <typename A1>
    void pushArgs(duk::Context &d, A1&& a) const {
        Type<typename std::decay<A1>::type>::push(d, std::forward<A1>(a));
    }

    template <typename A1, typename  A2, typename ... AA>
    void pushArgs(Context &d, A1 &&a1, A2 &&a2, AA && ... args) const {
        pushArgs<A1>(d, std::forward<A1>(a1));
        pushArgs<A2, AA...>(d, std::forward<A2>(a2), std::forward<AA>(args)...);
    }

    template <typename ... AA>
    void pushArgs(duk::Context &d, AA&& ... args) const {
        pushArgs(d, std::forward<AA>(args)...);
    }
};

template <class R, class ... A>
struct Type<std::function<R(A...)>> {
    static void push(duk::Context &d, std::function<R(A...)> const &val) {
        assert(false && "Push std::function to duktape stack is not implemented");
    }

    /**
     * Get function from stack at specified index
     * @param[in] d pointer to duktape context
     * @param[out] val value
     * @param[in] index value index at duktape stack
     */
    static void get(duk::Context &d, std::function<R(A...)> &val, int funcIndex) {
        // push function to heap stash
        int key = d.stashRef(funcIndex);

        duk_context *dukPtr = d.ptr();
        assignFunction(val, dukPtr, key);
    }

    template <class RR, class ... AA>
    static void assignFunction(std::function<RR(AA...)> &val, duk_context *dukPtr, int heapIdx) {
        JSFunction<RR, AA...> func(dukPtr, heapIdx);
        auto fn = [f = std::move(func)] (AA&& ... args) -> RR {
            try {
                return f.call(std::forward<AA>(args)...);
            }
            catch (ScriptEvaluationExcepton &e) {
                printf("%s", e.what());
                throw e;
            }
        };
        val = std::move(fn);
    }

    static constexpr bool isPrimitive() { return true; };

};

}
