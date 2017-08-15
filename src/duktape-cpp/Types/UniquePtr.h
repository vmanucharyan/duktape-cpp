#pragma once

#include <memory>
#include <cassert>

#include "../Utils/ClassInfo.h"
#include "../Utils/Helpers.h"
#include "../Utils/Inspect.h"

#include "../PushObjectInspector.h"

#include "../Box.h"
#include "../Type.h"
#include "../Context.h"


namespace duk {

namespace details {

// snippet from http://stackoverflow.com/questions/26377430/how-to-do-perform-a-dynamic-cast-with-a-unique-ptr
template <typename To, typename From>
std::unique_ptr<To> dynamic_unique_cast(std::unique_ptr<From>&& p) {
    if (To* cast = dynamic_cast<To*>(p.get()))
    {
        std::unique_ptr<To> result(cast);
        p.release();
        return result;
    }
    return std::unique_ptr<To>(nullptr);
}

template <class T, bool HasBase>
struct MakeUptrBox {
    static std::unique_ptr<BoxBase> make(std::unique_ptr<T> value);
    static void assign(BoxBase const &box, std::unique_ptr<T> &value);
};

template <class T>
struct MakeUptrBox<T, true> {
    static std::unique_ptr<BoxBase> make(std::unique_ptr<T> value) {
        return std::make_unique<Box<std::unique_ptr<typename BaseClass<T>::type>>>(std::move(value));
    }

    static void assign(BoxBase &box, std::unique_ptr<T> &value) {
        auto &b = box.as<Box<std::unique_ptr<typename BaseClass<T>::type>>>();

        assert(b.value());

        std::unique_ptr<typename BaseClass<T>::type> &v = b.value();
        std::unique_ptr<typename BaseClass<T>::type> bp(std::move(v));
        std::unique_ptr<T> cp = dynamic_unique_cast<T>(std::move(bp));

        if (cp) {
            value = std::move(cp);
        }
    }
};

template <class T>
struct MakeUptrBox<T, false> {
    typedef ClearType<T> TC;

    static std::unique_ptr<BoxBase> make(std::unique_ptr<TC> value) {
        return std::make_unique<Box<std::unique_ptr<TC>>>(std::move(value));
    }

    static void assign(BoxBase &box, std::unique_ptr<T> &value) {
        auto &b = dynamic_cast< Box<std::unique_ptr<T>> & >(box);
        value = std::move(b.value());
    }
};

}

template <class T>
struct Type<std::unique_ptr<T>> {
    static duk_ret_t finalizer(duk_context *d) {
        // get pointer to duk::Context
        duk_push_global_stash(d);
        duk_get_prop_string(d, -1, "self_ptr");
        duk::Context *self = reinterpret_cast<duk::Context*>(duk_get_pointer(d, -1));
        duk_pop_2(d);

        // get pointer to shared pointer
        duk_get_prop_string(d, 0, "\xff" "uptr_key");
        int boxKey = duk_get_int(d, -1);
        duk_pop(d);

        self->removeBox(boxKey);

        return 0;
    }

    static void push(duk::Context &d, std::unique_ptr<T> value) {
        assert(value);

        T * objPtr = value.get();

        std::unique_ptr<BoxBase> box = details::MakeUptrBox<T, BaseClass<T>::isDefined()>::make(std::move(value));

        int boxKey = d.storeBox(std::move(box));

        auto objIdx = duk_push_object(d);

        duk_push_int(d, boxKey);
        duk_put_prop_string(d, -2, "\xff" "uptr_key");

        duk_push_pointer(d, objPtr);
        duk_put_prop_string(d, -2, "\xff" "obj_ptr");

        duk_push_c_function(d, finalizer, 1);
        duk_set_finalizer(d, -2);

        details::PushObjectInspector i(d, objIdx);
        Inspect<T>::inspect(i);
    }

    static void get(duk::Context &d, std::unique_ptr<T> &value, int index) {
        duk_get_prop_string(d, index, "\xff" "uptr_key");
        int key = duk_get_int(d, -1);
        duk_pop(d);

        auto &box = d.getBox(key);

        details::MakeUptrBox<T, BaseClass<T>::isDefined()>::assign(box, value);
    }

    static constexpr bool isPrimitive() { return true; };
};

}
