#pragma once

#include <utility>

#include "./Utils/ClassInfo.h"
#include "./Utils/Helpers.h"
#include "./Utils/Inspect.h"

#include "Context.h"

#include "Type.h"
#include "Constructor.h"
#include "PushConstructorInspector.h"


namespace duk {

template <class T>
inline void Context::addGlobal(const char *name, T &&val) {
    duk_push_global_object(_ctx);
    push(std::forward<T>(val));
    duk_put_prop_string(_ctx, -2, name);
    duk_pop(_ctx);
}

template <class T>
inline void Context::push(T &&val) {
   Type<typename std::decay<T>::type>::push(*this, std::forward<T>(val));
}

template <class T>
inline void Context::registerClass() {
    duk_push_global_object(_ctx);

    auto namespaces = splitNamespaces(std::string(GetClassName<T>()));
    int depth = defNamespaces(namespaces);

    details::PushConstructorInspector i(*this);
    Inspect<T>::inspect(i);

    duk_put_prop_string(_ctx, -2, namespaces.back().c_str());
    duk_pop_n(_ctx, depth + 1);
}

template <class T>
inline void Context::evalString(T &res, const char *str) {
    duk_int_t ret = duk_peval_string(_ctx, str);
    if (ret != 0) {
        rethrowDukError();
    }

    Type<T>::get(*this, res, -1);
    duk_pop(_ctx);
}

namespace details {

class ConstantsInspector: public EmptyInspector {
public:
    ConstantsInspector(duk::Context &d, int objIdx) : _objIdx(objIdx),_d(d) { }

    template<typename T>
    void constant(const char *name, T value) {
        Type<T>::push(_d, value);
        duk_put_prop_string(_d, _objIdx, name);
    }

private:
    int _objIdx;
    duk::Context &_d;
};

}

template <class T>
inline void Context::registerEnum() {
    duk_push_global_object(_ctx);

    auto namespaces = splitNamespaces(std::string(GetClassName<T>()));
    int depth = defNamespaces(namespaces);

    int objIdx = duk_push_object(_ctx);
    details::ConstantsInspector i(*this, objIdx);
    Inspect<T>::inspect(i);
    duk_put_prop_string(_ctx, -2, namespaces.back().c_str());
    duk_pop(_ctx); // enum object

    duk_pop_n(_ctx, depth); // namespaces
}

}
