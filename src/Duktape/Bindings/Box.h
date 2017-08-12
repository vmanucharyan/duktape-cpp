#pragma once

#include <type_traits>

namespace duk {

class BoxBase {
public:
    virtual ~BoxBase() = 0;

    template <class T>
    T & as() {
        static_assert(std::is_base_of<BoxBase, T>::value, "invalid type cast");
        return dynamic_cast<T&>(*this);
    }

    template <class T>
    T const & as() const {
        static_assert(std::is_base_of<BoxBase, T>::value, "invalid type cast");
        return dynamic_cast<T const &>(*this);
    }
};

inline BoxBase::~BoxBase() {};

template <class T>
class Box: public BoxBase {
public:
    explicit Box(T value): _value(std::move(value)) {}

    T const & value() const { return _value; }
    T & value() { return _value; }

private:
    T _value;
};

}
