#pragma once

#include <type_traits>
#include <vector>
#include <string>

namespace duk {

template <class A> using ClearType = typename std::decay<A>::type;

/**
 * Checks wheter class T has `inspect` method
 */
template <typename T>
struct HasInspectMethod {
    struct dummy { /* something */ };

    template <typename C, typename P>
    static auto test(P * p) -> decltype(C::inspect(*p), std::true_type());

    template <typename, typename>
    static std::false_type test(...);

    typedef decltype(test<T, dummy>(nullptr)) type;
    static const bool value = std::is_same<std::true_type, decltype(test<T, dummy>(nullptr))>::value;
};

std::vector<std::string> splitNamespaces(std::string const &className);

}
