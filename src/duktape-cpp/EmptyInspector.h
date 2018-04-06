#pragma once

#include <memory>

namespace duk { namespace details {

class EmptyInspector {
public:
    template <class C, class A> using Getter = A (C::*)() const;
    template <class C, class A> using Setter = void (C::*)(A a);

    template <typename T>
    void constant(const char *name, T value) {}

    template <class C, class A>
    void property(const char *name, Getter<C, A> getter, Setter<C, A> setter) {}

    template <class C, class A>
    void property(const char *name, Getter<C, A> getter) {}

    template <class C, class R, class ... A>
    void method(const char *name, R(C::*method)(A...)) {}

    template <class C, class R, class ... A>
    void method(const char *name, R(C::*method)(A...) const) {}

    template <class R, class ... A>
    void static_method(const char *name, R(*method)(A...)) {}

    template <class C, class ... A>
    void construct(std::shared_ptr<C> (*constructor) (A...)) {}

    template <class C, class ... A>
    void construct(std::unique_ptr<C> (*constructor) (A...)) {}
};

}}
