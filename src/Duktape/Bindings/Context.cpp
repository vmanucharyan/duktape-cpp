#include "Context.h"

#include <sstream>
#include <cassert>

#include "Exceptions.h"

namespace duk {

Context::Context(std::string const &scriptId) : _ctx(nullptr), _scriptId(scriptId) {
    _ctx = duk_create_heap_default();
    assignSelf();
}

Context::~Context() {
    if (_ctx) {
        duk_destroy_heap(_ctx);
    }
}

Context::Context(Context &&that) noexcept : _ctx(that._ctx), _scriptId(that._scriptId) {
    that._ctx = nullptr;
    assignSelf();
}

Context &Context::operator=(Context &&that) noexcept {
    if (this == &that) {
        return *this;
    }

    this->_ctx = that._ctx;
    this->_scriptId = std::move(that._scriptId);
    that._ctx = nullptr;

    assignSelf();

    return *this;
}

int Context::storeBox(std::unique_ptr<BoxBase> box) {
    _boxCounter += 1;
    _boxes[_boxCounter] = std::move(box);
    return _boxCounter;
}

BoxBase & Context::getBox(int key) const {
    return *_boxes.at(key);
}

void Context::removeBox(int key) {
    _boxes.erase(key);
}

int Context::defNamespaces(std::vector<std::string> const &namespaces) {
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

void Context::evalStringNoRes(const char *str) {
    duk_int_t ret = duk_peval_string_noresult(_ctx, str);
    if (ret != 0) {
        rethrowDukError();
    }
}

void Context::rethrowDukError() {
    throw ScriptEvaluationExcepton(duk_safe_to_string(_ctx, -1));
}

Context& Context::GetSelfFromContext(duk_context *d) {
    duk_push_global_stash(d);
    duk_get_prop_string(d, -1, "self_ptr");
    duk::Context *ctx = reinterpret_cast<duk::Context*>(duk_get_pointer(d, -1));
    duk_pop_2(d);
    return *ctx;
}

void Context::assignSelf() {
    duk_push_global_stash(_ctx);
    duk_push_pointer(_ctx, this);
    duk_put_prop_string(_ctx, -2, "self_ptr");
    duk_pop(_ctx);
}

int Context::stashRef(int stackIndex) {
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

void Context::unstashRef(int refKey) {
    duk_push_global_stash(_ctx);

    assert(duk_has_prop_string(_ctx, -1, "refs"));
    duk_get_prop_string(_ctx, -1, "refs");

    duk_del_prop_index(_ctx, -1, duk_uarridx_t(refKey));

    duk_pop_2(_ctx);
}

void Context::getRef(int key) {
    duk_push_global_stash(_ctx);

    assert(duk_has_prop_string(_ctx, -1, "refs"));
    duk_get_prop_string(_ctx, -1, "refs");

    assert(duk_has_prop_index(_ctx, -1, duk_uarridx_t(key)));
    duk_get_prop_index(_ctx, -1, duk_uarridx_t(key));

    duk_swap_top(_ctx, -3);

    duk_pop_2(_ctx);
}

}
