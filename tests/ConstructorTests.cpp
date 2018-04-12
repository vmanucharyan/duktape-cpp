#include <catch/catch.hpp>

#include <iostream>

#include <utility>

#include <duktape-cpp/DuktapeCpp.h>

#include "TestTypes.h"

using namespace duk;

namespace ConstructorTests {

class SimpleConstructible {
public:
    static std::shared_ptr<SimpleConstructible> Construct(float fieldOne, Vec2 const &fieldTwo) {
        return std::make_shared<SimpleConstructible>(fieldOne, fieldTwo);
    }

    SimpleConstructible(): _fieldOne(0), _fieldTwo(0.0f, 0.0f) {}

    SimpleConstructible(float fieldOne, Vec2 const &fieldTwo)
        : _fieldOne(fieldOne), _fieldTwo(fieldTwo) {}

    float fieldOne() const { return _fieldOne; }
    void setFieldOne(float value) { _fieldOne = value; }

    Vec2 const & fieldTwo() const { return _fieldTwo; }
    void setFieldTwo(Vec2 const &value) { _fieldTwo = value; }

    template <class Inspector>
    static void inspect(Inspector &i) {
        i.construct(&SimpleConstructible::Construct);
        i.property("fieldOne", &SimpleConstructible::fieldOne, &SimpleConstructible::setFieldOne);
        i.property("fieldTwo", &SimpleConstructible::fieldTwo, &SimpleConstructible::setFieldTwo);
    }

private:
    float _fieldOne;
    Vec2 _fieldTwo;
};

class ComplexConstructible {
public:
    static std::shared_ptr<ComplexConstructible> Construct(std::shared_ptr<SimpleConstructible> const &fieldOne, Vec2 const &fieldTwo) {
        return std::make_shared<ComplexConstructible>(fieldOne, fieldTwo);
    }

    ComplexConstructible() {}

    ComplexConstructible(std::shared_ptr<SimpleConstructible> const &fieldOne, Vec2 const &fieldTwo)
        : _fieldOne(fieldOne), _fieldTwo(fieldTwo) {}

    std::shared_ptr<SimpleConstructible> const & fieldOne() const { return _fieldOne; }
    void setFieldOne(std::shared_ptr<SimpleConstructible> const &value) { _fieldOne = value; }

    Vec2 const & fieldTwo() const { return _fieldTwo; }
    void setFieldTwo(Vec2 const &value) { _fieldTwo = value; }

    template <class Inspector>
    static void inspect(Inspector &i) {
        i.construct(&ComplexConstructible::Construct);
        i.property("fieldOne", &ComplexConstructible::fieldOne, &ComplexConstructible::setFieldOne);
        i.property("fieldTwo", &ComplexConstructible::fieldTwo, &ComplexConstructible::setFieldTwo);
    }

private:
    std::shared_ptr<SimpleConstructible> _fieldOne;
    Vec2 _fieldTwo;
};

class NoArgsConstructible {
public:
    static std::shared_ptr<NoArgsConstructible> Construct() {
        return std::make_shared<NoArgsConstructible>();
    }

    NoArgsConstructible() : _field(Vec2(1, 2)) {}

    Vec2 const & field() const { return _field; }
    void setField(Vec2 const &value) { _field = value; }

    template <class Inspector>
    static void inspect(Inspector &i) {
        i.construct(&NoArgsConstructible::Construct);
        i.property("field", &NoArgsConstructible::field, &NoArgsConstructible::setField);
    }

private:
    Vec2 _field;
};
}

TEST_CASE("PushConstructorInspector", "[duktape]") {
    using namespace ConstructorTests;

    duk::Context d;

    SECTION("should handle constructors with no arguments") {
        const char script[] =
            "var a = new NoArgsConstructible();\n"
            "a.field = { x: -1, y: -2 };\n"
            "a\n";

        duk_push_global_object(d);
        duk::details::PushConstructorInspector i(d);
        NoArgsConstructible().inspect(i);
        duk_put_prop_string(d, -2, "NoArgsConstructible");

        auto evalRes = duk_peval_string(d, script);
        if (evalRes != 0) {
            std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
        }
        REQUIRE(evalRes == 0);

        std::shared_ptr<NoArgsConstructible> obj;
        duk::Type<std::shared_ptr<NoArgsConstructible>>::get(d, obj, -1);

        REQUIRE(obj->field() == Vec2(-1, -2));
    }

    SECTION("should handle constructors with simple arguments") {
        const char script[] = "new SimpleConstructible(1.5, {x: 1, y: 2});";

        duk_push_global_object(d);
        duk::details::PushConstructorInspector i(d);
        SimpleConstructible().inspect(i);
        duk_put_prop_string(d, -2, "SimpleConstructible");

        auto evalRes = duk_peval_string(d, script);
        if (evalRes != 0) {
            std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
        }
        REQUIRE(evalRes == 0);

        std::shared_ptr<SimpleConstructible> obj;
        duk::Type<std::shared_ptr<SimpleConstructible>>::get(d, obj, -1);

        REQUIRE(obj->fieldOne() == 1.5f);
        REQUIRE(obj->fieldTwo() == Vec2(1, 2));
    }

    SECTION("should handle constructors with complex arguments") {
        const char script[] =
            "var simple = new SimpleConstructible(1.5, {x: 1, y: 2});\n"
            "new ComplexConstructible(simple, {x: 5, y: -6})";

        duk_push_global_object(d);

        duk::details::PushConstructorInspector simpleInspector(d);
        SimpleConstructible().inspect(simpleInspector);
        duk_put_prop_string(d, -2, "SimpleConstructible");

        duk::details::PushConstructorInspector complexInspector(d);
        ComplexConstructible().inspect(complexInspector);
        duk_put_prop_string(d, -2, "ComplexConstructible");

        auto evalRes = duk_peval_string(d, script);
        if (evalRes != 0) {
            std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
        }
        REQUIRE(evalRes == 0);

        std::shared_ptr<ComplexConstructible> obj;
        duk::Type<std::shared_ptr<ComplexConstructible>>::get(d, obj, -1);

        REQUIRE(obj->fieldOne()->fieldOne() == 1.5);
        REQUIRE(obj->fieldOne()->fieldTwo() == Vec2(1, 2));
        REQUIRE(obj->fieldTwo() == Vec2(5, -6));
    }
}
