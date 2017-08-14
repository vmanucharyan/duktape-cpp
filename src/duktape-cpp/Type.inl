#include "Type.h"

#include <cassert>

#include <duktape.h>

#include <duktape-cpp/PushObjectInspector.h>

#include "./Utils/Helpers.h"
#include "./Utils/Inspect.h"

namespace duk {

template <class T>
inline void Type<T>::push(duk::Context &d, T const &value) {
    auto objIdx = duk_push_object(d);
    duk_push_pointer(d, const_cast<T*>(&value));
    duk_put_prop_string(d, -2, "\xff" "obj_ptr");
    details::PushObjectInspector i(d, objIdx);
    Inspect<T>::inspect(i);
}

template <class T>
inline void Type<T>::get(duk::Context &d, T &value, int objIdx) {
    static_assert(std::is_copy_constructible<T>::value, "object must be copy constructible");

    duk_get_prop_string(d, objIdx, "\xff" "obj_ptr");
    T *obj = reinterpret_cast<T*>(duk_get_pointer(d, -1));
    duk_pop(d);

    assert(obj);

    value = *obj;
}

}
