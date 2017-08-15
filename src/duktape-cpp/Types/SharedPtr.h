#pragma once

#include <memory>

#include "../Utils/ClassInfo.h"
#include "../Utils/Helpers.h"
#include "../Utils/Inspect.h"

#include "../PushObjectInspector.h"

#include "../Box.h"
#include "../Type.h"
#include "../Context.h"

namespace duk {

namespace details {

template <class T, bool HasBase>
struct SptrBox {
    static std::unique_ptr<BoxBase> make(std::shared_ptr<T> const &value);
    static void assign(BoxBase const &box, std::shared_ptr<T> &value);
};

template <class T>
struct SptrBox<T, true> {
    static std::unique_ptr<BoxBase> make(std::shared_ptr<T> const &value) {
        return std::make_unique<Box<std::shared_ptr<typename BaseClass<T>::type>>>(value);
    }

    static void assign(BoxBase const &box, std::shared_ptr<T> &value) {
        auto const &b = box.as<Box<std::shared_ptr<typename BaseClass<T>::type>>>();
        value = std::dynamic_pointer_cast<T>(b.value());
    }
};

template <class T>
struct SptrBox<T, false> {
    typedef ClearType<T> TC;

    static std::unique_ptr<BoxBase> make(std::shared_ptr<TC> const &value) {
        return std::make_unique<Box<std::shared_ptr<TC>>>(value);
    }

    static void assign(BoxBase const &box, std::shared_ptr<T> &value) {
        auto const &b = dynamic_cast< Box<std::shared_ptr<T>> const & >(box);
        value = b.value();
    }
};

}



template <class T>
struct Type<std::shared_ptr<T>> {
    static duk_ret_t finalizer(duk_context *d) {
        // get pointer to duk::Context
        duk_push_global_stash(d);
        duk_get_prop_string(d, -1, "self_ptr");
        duk::Context *self = reinterpret_cast<duk::Context*>(duk_get_pointer(d, -1));
        duk_pop_2(d);

        // get pointer to shared pointer
        duk_get_prop_string(d, 0, "\xff" "sptr_key");
        int boxKey = duk_get_int(d, -1);
        duk_pop(d);

        self->removeBox(boxKey);

        return 0;
    }

    static void push(duk::Context &d, std::shared_ptr<T> const &value) {
        if (!value) {
            duk_push_null(d);
            return;
        }

        std::unique_ptr<BoxBase> box = details::SptrBox<T, BaseClass<T>::isDefined()>::make(value);

        int boxKey = d.storeBox(std::move(box));

        auto objIdx = duk_push_object(d);

        duk_push_int(d, boxKey);
        duk_put_prop_string(d, -2, "\xff" "sptr_key");

        duk_push_pointer(d, value.get());
        duk_put_prop_string(d, -2, "\xff" "obj_ptr");

        duk_push_c_function(d, finalizer, 1);
        duk_set_finalizer(d, -2);

        details::PushObjectInspector i(d, objIdx);
        Inspect<T>::inspect(i);
    }

    static void get(duk::Context &d, std::shared_ptr<T> &value, int index) {
        if (duk_is_null_or_undefined(d, index)) {
            value = std::shared_ptr<T>();
            return;
        }

        duk_get_prop_string(d, index, "\xff" "sptr_key");
        int key = duk_get_int(d, -1);
        duk_pop(d);

        auto const &box = d.getBox(key);
        details::SptrBox<T, BaseClass<T>::isDefined()>::assign(box, value);
    }

    static constexpr bool isPrimitive() { return true; };
};

}
