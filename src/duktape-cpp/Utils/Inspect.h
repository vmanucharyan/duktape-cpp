#pragma once

#include "Helpers.h"

namespace duk {

template <class T>
struct Inspect {
    template <class I>
    static void inspect(I &i);
};

namespace details {

template<class Base>
struct InspectBase {
    template<class I>
    static void inspect(I &i) {
        Inspect<Base>::inspect(i);
    }
};

template<>
struct InspectBase<void> {
    template<class I>
    static void inspect(I &i) {
        // do nothing
    }
};

}

template <class T>
template <class I>
inline void Inspect<T>::inspect(I &i) {
    static_assert(
        HasInspectMethod<T>::value,
        "class must have `inspect` method or specialized `Inspect` struct template"
    );
    T::inspect(i);
    details::InspectBase<BaseOf<T>>::inspect(i);
};

}
