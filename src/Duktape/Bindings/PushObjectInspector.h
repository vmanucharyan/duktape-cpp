#pragma once

#include "EmptyInspector.h"

#include <duktape.h>

namespace duk {

class Context;

namespace details {

class PushObjectInspector: public EmptyInspector {
public:
    PushObjectInspector(duk::Context &d, int objIdx): _d(d), _objIdx(objIdx) {}

    template <class C, class A> using Getter = A (C::*)() const;
    template <class C, class A> using Setter = void (C::*)(A a);

    template <class C, class A>
    void property(const char *name, Getter<C, A> getter, Setter<C, A> setter);

    template <class C, class A>
    void property(const char *name, Getter<C, A> getter);

    template <class C, class R, class ... A>
    void method(const char *name, R(C::*method)(A...));

    template <class C, class R, class ... A>
    void method(const char *name, R(C::*method)(A...) const);

private:
    duk::Context &_d;
    int _objIdx;
};

}}
