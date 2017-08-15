#pragma once

#include <utility>
#include <cassert>

#include "./Utils/ClassInfo.h"
#include "./Utils/Helpers.h"
#include "./Utils/Inspect.h"

#include "Context.h"
#include "Type.h"
#include "Constructor.h"
#include "PushConstructorInspector.h"
#include "Exceptions.h"

namespace duk {

static void fatal_handler(void *udata, const char *msg) {
    fprintf(stderr, "*** FATAL ERROR: %s\n", (msg ? msg : "no message"));
    fflush(stderr);
    throw DuktapeException(msg);
}

inline Context::Context(std::string const &scriptId) : _ctx(nullptr), _scriptId(scriptId) {
    _ctx = duk_create_heap(NULL, NULL, NULL, this, fatal_handler);
    assignSelf();
}

inline Context::~Context() {
    if (_ctx) {
        duk_destroy_heap(_ctx);
    }
}

inline Context::Context(Context &&that) noexcept : _ctx(that._ctx), _scriptId(that._scriptId) {
    that._ctx = nullptr;
    assignSelf();
}

inline Context &Context::operator=(Context &&that) noexcept {
    if (this == &that) {
        return *this;
    }

    this->_ctx = that._ctx;
    this->_scriptId = std::move(that._scriptId);
    that._ctx = nullptr;

    assignSelf();

    return *this;
}

inline int Context::storeBox(std::unique_ptr<BoxBase> box) {
    _boxCounter += 1;
    _boxes[_boxCounter] = std::move(box);
    return _boxCounter;
}

inline BoxBase & Context::getBox(int key) const {
    return *_boxes.at(key);
}

inline void Context::removeBox(int key) {
    _boxes.erase(key);
}

inline int Context::defNamespaces(std::vector<std::string> const &namespaces) {
    int depth = 0;

    if (namespaces.size() > 1) {
        for (int i = 0; i < namespaces.size() - 1; i++) {
            auto const &ns = namespaces[i];

            duk_get_prop_string(_ctx, -1, ns.c_str());
            bool isDefined = ! bool(duk_is_undefined(_ctx, -1));
            duk_pop(_ctx);

            if (!isDefined) {
                duk_push_object(_ctx);
                duk_put_prop_string(_ctx, -2, ns.c_str());
            }

            duk_get_prop_string(_ctx, -1, ns.c_str());
            depth += 1;
        }
    }

    return depth;
}

inline void Context::evalStringNoRes(const char *str) {
    duk_int_t ret = duk_peval_string_noresult(_ctx, str);
    if (ret != 0) {
        rethrowDukError();
    }
}

inline void Context::rethrowDukError() {
    printf("%d\n", duk_get_top(_ctx));
    const char *errorMessage = duk_safe_to_string(_ctx, -1);
    throw ScriptEvaluationExcepton(std::string(errorMessage));
}

inline Context& Context::GetSelfFromContext(duk_context *d) {
    duk_push_global_stash(d);
    duk_get_prop_string(d, -1, "self_ptr");
    duk::Context *ctx = reinterpret_cast<duk::Context*>(duk_get_pointer(d, -1));
    duk_pop_2(d);
    return *ctx;
}

inline void Context::assignSelf() {
    duk_push_global_stash(_ctx);
    duk_push_pointer(_ctx, this);
    duk_put_prop_string(_ctx, -2, "self_ptr");
    duk_pop(_ctx);
}

inline int Context::stashRef(int stackIndex) {
    int key = _objectRefCounter;
    ++ _objectRefCounter;

    duk_push_global_stash(_ctx);
    if (!duk_has_prop_string(_ctx, -1, "refs")) {
        duk_push_object(_ctx);
        duk_put_prop_string(_ctx, -2, "refs");
    }

    duk_get_prop_string(_ctx, -1, "refs");
    duk_dup(_ctx, stackIndex < 0 ? stackIndex - 2 : stackIndex);
    duk_put_prop_index(_ctx, -2, (duk_uarridx_t) key);

    duk_pop_2(_ctx); // refs + global stash

    return key;
}

inline void Context::unstashRef(int refKey) {
    duk_push_global_stash(_ctx);

    assert(duk_has_prop_string(_ctx, -1, "refs"));
    duk_get_prop_string(_ctx, -1, "refs");

    duk_del_prop_index(_ctx, -1, duk_uarridx_t(refKey));

    duk_pop_2(_ctx);
}

inline void Context::getRef(int key) {
    duk_push_global_stash(_ctx);

    assert(duk_has_prop_string(_ctx, -1, "refs"));
    duk_get_prop_string(_ctx, -1, "refs");

    assert(duk_has_prop_index(_ctx, -1, duk_uarridx_t(key)));
    duk_get_prop_index(_ctx, -1, duk_uarridx_t(key));

    duk_swap_top(_ctx, -3);

    duk_pop_2(_ctx);
}

template <class T>
inline void Context::addGlobal(const char *name, T &&val) {
    duk_push_global_object(_ctx);
    push(std::forward<T>(val));
    duk_put_prop_string(_ctx, -2, name);
    duk_pop(_ctx);
}

template <class T>
inline void Context::push(T &&val) {
   Type<ClearType<T>>::push(*this, std::forward<T>(val));
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
inline void Context::getGlobal(const char *name, T &res) {
    duk_push_global_object(_ctx);
    duk_get_prop_string(_ctx, -1, name);

    if (duk_is_undefined(_ctx, -1)) {
        duk_pop_2(_ctx);
        throw KeyError(std::string(name) + " is undefined");
    }

    Type<T>::get(*this, res, -1);

    duk_pop_2(_ctx);
}

}
