#include <catch/catch.hpp>

#include <Engine/Duktape/Bindings/Prelude.h>

using namespace engine;

namespace EngineTypesTests {

class ITestClass {
public:
    virtual bool mouse(MouseButton btn) = 0;
    virtual bool key(KeyCode kc) = 0;

    template <class I>
    static void inspect(I &i) {
        i.method("mouse", &ITestClass::mouse);
        i.method("key", &ITestClass::key);
    }
};

class TestClass: public ITestClass {
public:
    bool mouse(MouseButton btn) override {
        return btn == MouseButton::MidButton;
    }

    bool key(KeyCode kc) override {
        return kc == KeyCode::A;
    }

    template <class I>
    static void inspect(I &i) {
        i.method("mouse", &TestClass::mouse);
        i.method("key", &TestClass::key);
    }
};

}

TEST_CASE("engine types tests", "[duktape]") {
    using namespace EngineTypesTests;

    duk::Context d;

    SECTION("MouseButton") {
        MouseButton expected = MouseButton::MidButton;

        duk::Type<MouseButton>::push(d, expected);

        MouseButton actual = MouseButton::RightButton;
        duk::Type<MouseButton>::get(d, actual, -1);

        REQUIRE(actual == expected);

        SECTION("call method from js") {
            sp<ITestClass> tc = makeShared<TestClass>();

            d.addGlobal("Test", tc);
            d.registerEnum<KeyCode>();
            const char script[] = "Test.key(engine.KeyCode.A)";

            bool res = false;
            d.evalString(res, script);

            REQUIRE(res);
        }
    }

    SECTION("KeyCode") {
        KeyCode expected = KeyCode::Backslash;

        duk::Type<KeyCode>::push(d, expected);

        KeyCode actual = KeyCode::A;
        duk::Type<KeyCode>::get(d, actual, -1);

        REQUIRE(actual == expected);
    }
}
