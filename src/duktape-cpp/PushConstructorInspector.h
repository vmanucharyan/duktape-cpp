#pragma once

#include "Context.h"

#include "EmptyInspector.h"
#include "Constructor.h"
#include "Method.h"

namespace duk { namespace details {

class PushConstructorInspector: public EmptyInspector {
public:
    PushConstructorInspector(duk::Context &ctx)
        : _ctx(ctx) {}

    template <class C, class ... A>
    void construct(std::shared_ptr<C> (*constructor) (A...)) {
        Constructor<C, A...>::push(_ctx, constructor);
    }

    template <class C, class ... A>
    void construct(std::unique_ptr<C> (*constructor) (A...)) {
        ConstructorUnique<C, A...>::push(_ctx, constructor);
    }

    template <class A>
    void static_property(const char *name, StaticGetter<A> getter, StaticSetter<A> setter) {
        duk_push_string(_ctx, name);
        PushMethod(_ctx, getter);
        PushMethod(_ctx, setter);
        duk_def_prop(
            _ctx,
            -4,
            DUK_DEFPROP_HAVE_GETTER |
            DUK_DEFPROP_HAVE_SETTER
        );
    }

    template <class A>
    void static_property(const char *name, StaticGetter<A> getter) {
        duk_push_string(_ctx, name);
        PushMethod(_ctx, getter);
        duk_def_prop(
            _ctx,
            -3,
            DUK_DEFPROP_HAVE_GETTER
        );
    }

    template <class R, class ... A>
    void static_method(const char *name, R(*method)(A...)) {
        PushMethod(_ctx, method);
        duk_put_prop_string(_ctx, -2, name);
    }

private:
    duk::Context &_ctx;
};

}}
