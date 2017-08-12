#pragma once

#include "Helpers.h"

namespace duk {

template <class T>
struct Inspect {
    template <class I>
    static void inspect(I &i) {
        static_assert(
            HasInspectMethod<T>::value,
            "class must have `inspect` method or specialized `Inspect` struct template"
        );
        T::inspect(i);
    }
};

}
