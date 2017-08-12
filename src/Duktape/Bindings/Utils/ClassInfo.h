#pragma once

#include <string>

namespace duk {

/**
 * @brief Class name as string
 */
template <typename T>
struct ClassName {
    static constexpr bool isDefined() { return false; };
    static constexpr auto value = "UnknownClass";
};

template <typename T>
struct ShortClassName {
    static constexpr bool isDefined() { return false; }
    static constexpr auto value = ClassName<T>::value;
};

template <class T>
constexpr auto GetClassName() {
    static_assert(ClassName<T>::isDefined(), "ClassName not defined");
    return ClassName<T>::value;
}

template <class T>
struct BaseClass {
    static constexpr bool isDefined() { return false; }
    typedef void type;
};

template <class T>
class TypeName {
    const char * typeName() const {
        return GetClassName<T>();
    }
};

template <class T>
struct IsPolymorphic {
    static constexpr bool value() { return false; }
};

}

/**
 * @brief Defines class name.
 */
#define DEF_CLASS_NAME(T) \
    namespace duk { \
    template <> \
    struct ClassName<T> { \
        static constexpr bool isDefined() { return true; }; \
        static constexpr auto value = #T; \
    };}

/**
 * @brief Defines class short name.
 */
#define DEF_SHORT_NAME(T, shortName) \
    namespace duk { \
    template <> \
    struct ShortClassName<T> { \
        static constexpr bool isDefined() { return true; } \
        static constexpr auto value = shortName; \
    };}

#define DEF_BASE_CLASS(Type, Base) \
    namespace duk { \
    template <> \
    struct BaseClass<Type> { \
        static constexpr bool isDefined() { return true; } \
        typedef Base type; \
    };}

#define DEF_POLYMORPHIC(T) \
    namespace duk { \
    template <> struct IsPolymorphic<T> { \
        static constexpr bool value() { return true; } \
    };}
