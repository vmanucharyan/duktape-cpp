#pragma once

#include "Context.h"

#include "EmptyInspector.h"
#include "Constructor.h"

namespace duk { namespace details {

class PushConstructorInspector: public EmptyInspector {
public:
    PushConstructorInspector(duk::Context &ctx)
        : _ctx(ctx) {}

    template <class C, class ... A>
    void construct(std::shared_ptr<C> (*constructor) (A...)) {
        if (!_hasConstructor) {
            _hasConstructor = true;
            Constructor<C, A...>::push(_ctx, constructor);
        }
    }

    template <class C, class ... A>
    void construct(std::unique_ptr<C> (*constructor) (A...)) {
        if (!_hasConstructor) {
            _hasConstructor = true;
            ConstructorUnique<C, A...>::push(_ctx, constructor);
        }
    }

private:
    duk::Context &_ctx;
    bool _hasConstructor = false;
};

}}
