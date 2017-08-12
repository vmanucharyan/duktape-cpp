#pragma once

#include <Engine/EngineStd.h>

#include <duktape.h>
#include <map>

namespace engine {

class ActorComponentBase;

namespace duk {

class Context;

namespace details {

template <class T>
struct Component {
    static void registerComponent(duk::Context &d, const char *name, int objIdx);
    static duk_ret_t detach(duk_context *d);
};

} // details

} // duk

} // engine
