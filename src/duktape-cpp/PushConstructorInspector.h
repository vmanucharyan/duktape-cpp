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

    template <class R, class ... A>
    inline void static_method(const char *name, R(*method)(A...)) {
        PushMethod(_ctx, method);
        duk_put_prop_string(_ctx, -2, name);
    }

private:
    duk::Context &_ctx;
};

}}
