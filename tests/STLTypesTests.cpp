#include <catch/catch.hpp>

#include <iostream>

#include <duktape.h>

#include <duktape-cpp/DuktapeCpp.h>

#include "TestTypes.h"

using namespace duk;

namespace STLTypesTests {

class PositionComponent {
public:
    static std::shared_ptr<PositionComponent> Construct(Vec3 const &pos) {
        return std::make_shared<PositionComponent>(pos);
    }

    PositionComponent(Vec3 const &pos): _pos(pos) {}

    Vec3 const & pos() const { return _pos; }
    void setPos(Vec3 const &value) { _pos = value; }

    template <class I>
    static void inspect(I &i) {
        i.construct(&PositionComponent::Construct);
        i.property("pos", &PositionComponent::pos, &PositionComponent::setPos);
    }

private:
    Vec3 _pos;
};

}

TEST_CASE("STL Types") {
    using namespace STLTypesTests;

    duk::Context d;

    SECTION("std::string") {
        std::string s("some string");

        duk::Type<std::string>::push(d, s);

        std::string popped;
        duk::Type<std::string>::get(d, popped, -1);

        duk_pop(d);

        REQUIRE(s == popped);
    }

    SECTION("std::vector of primitive types") {
        std::vector<int> v { 123, 124, -321 };
        duk::Type<std::vector<int>>::push(d, v);

        std::vector<int> popped;
        duk::Type<std::vector<int>>::get(d, popped, -1);

        duk_pop(d);

        REQUIRE(v == popped);

        SECTION("stack top is at 0") {
            REQUIRE(duk_get_top(d) == 0);
        }
    }

    SECTION("std::vector of objects") {
        std::vector<std::shared_ptr<PositionComponent>> v {
            std::make_shared<PositionComponent>(Vec3(1.0f, 2.0f, 3.0f)),
            std::make_shared<PositionComponent>(Vec3(4.0f, 5.0f, 6.0f))
        };

        duk::Type<std::vector<std::shared_ptr<PositionComponent>>>::push(d, v);

        std::vector<std::shared_ptr<PositionComponent>> popped;
        duk::Type<std::vector<std::shared_ptr<PositionComponent>>>::get(d, popped, -1);

        duk_pop(d);

        REQUIRE(v == popped);

        SECTION("stack top is at 0") {
            REQUIRE(duk_get_top(d) == 0);
        }
    }
}
