#include "../Type.h"

#include <tuple>

namespace duk {

template <typename ... A>
struct Type<std::tuple<A...>> {
    static void push(duk::Context &d, std::tuple<A...> const &val) {
        duk_push_array(d);
        pushElement<0, A...>(d, val);
    }

    static void get(duk::Context &d, std::tuple<A...> &val, int index) {
        duk_enum(d, index, DUK_ENUM_ARRAY_INDICES_ONLY);
        getElement<0, A...>(d, val);
        duk_pop(d);
    }

    static constexpr bool isPrimitive() { return true; };

private:
    template <int idx, typename AA, typename BB, typename ... CC>
    static void getElement(duk::Context &d, std::tuple<A...> &val) {
        getElement<idx, AA>(d, val);
        getElement<idx + 1, BB, CC...>(d, val);
    };

    template <int idx, typename AA>
    static void getElement(duk::Context &d, std::tuple<A...> &val) {
        duk_next(d, -1, 1);
        AA v;
        Type<AA>::get(d, v, -1);
        std::get<idx>(val) = v;
        duk_pop_2(d);
    }

    template <int idx, typename AA, typename BB, typename ... CC>
    static void pushElement(duk::Context &d, std::tuple<A...> const &val) {
        pushElement<idx, AA>(d, val);
        pushElement<idx + 1, BB, CC...>(d, val);
    };

    template <int idx, typename AA>
    static void pushElement(duk::Context &d, std::tuple<A...> const &val) {
        duk::Type<AA>::push(d, std::get<idx>(val));
        duk_put_prop_index(d, -2, static_cast<duk_uarridx_t>(idx));
    };
};

}
