#pragma once

#include "PushConstructorInspector.h"

#include "Method.h"

namespace duk { namespace details {

template <class A>
void PushConstructorInspector::static_property(const char *name, StaticGetter<A> getter, StaticSetter<A> setter) {
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
void PushConstructorInspector::static_property(const char *name, StaticGetter<A> getter) {
    duk_push_string(_ctx, name);
    PushMethod(_ctx, getter);
    duk_def_prop(
        _ctx,
        -3,
        DUK_DEFPROP_HAVE_GETTER
    );
}

template <class R, class ... A>
void PushConstructorInspector::static_method(const char *name, R(*method)(A...)) {
    PushMethod(_ctx, method);
    duk_put_prop_string(_ctx, -2, name);
}

}}
