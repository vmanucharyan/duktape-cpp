#pragma once

#include "PushObjectInspector.h"

#include "Method.h"

namespace duk { namespace details {

template <class C, class A>
inline void PushObjectInspector::property(const char *name, Getter<C, A> getter, Setter<C, A> setter) {
    duk_push_string(_d, name);
    PushMethod(_d, getter);
    PushMethod(_d, setter);
    duk_def_prop(
        _d,
        _objIdx,
        DUK_DEFPROP_HAVE_GETTER |
        DUK_DEFPROP_HAVE_SETTER
    );
}

template <class C, class A>
inline void PushObjectInspector::property(const char *name, Getter<C, A> getter) {
    duk_push_string(_d, name);
    PushMethod(_d, getter);
    duk_def_prop(
        _d,
        _objIdx,
        DUK_DEFPROP_HAVE_GETTER
    );
}

template <class C, class R, class ... A>
inline void PushObjectInspector::method(const char *name, R(C::*method)(A...)) {
    PushMethod(_d, method);
    duk_put_prop_string(_d, _objIdx, name);
}

template <class C, class R, class ... A>
inline void PushObjectInspector::method(const char *name, R(C::*method)(A...) const) {
    PushMethod(_d, method);
    duk_put_prop_string(_d, _objIdx, name);
}

}}
