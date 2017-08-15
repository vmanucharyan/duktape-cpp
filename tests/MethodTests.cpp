#include <catch/catch.hpp>
#include <catch/fakeit.hpp>

#include <iostream>

#include <duktape.h>

#include <duktape-cpp/Utils/ClassInfo.h>

#include <duktape-cpp/Context.inl>
#include <duktape-cpp/Types/All.h>
#include <duktape-cpp/Method.h>
#include <duktape-cpp/PushObjectInspector.inl>

#include "TestTypes.h"

using namespace duk;
using namespace fakeit;

namespace MethodTests {
class ITest2 {
public:
    virtual ~ITest2() {}

    virtual Vec2 someMethod(float number, Vec3 vector) {
        return Vec2(-6, 2);
    }

    template <class I>
    static void inspect(I &i) {
        i.method("someMethod", &ITest2::someMethod);
    }
};


class Test2: public ITest2 {
public:
    template <class I>
    static void inspect(I &i) {
        // i.method("someMethod", &ITest2::someMethod);
    }
};


class ITest {
public:
    virtual ~ITest() {}

    virtual int intNoArgs() = 0;
    virtual float floatSimpleArgs(float a, int b) = 0;
    virtual void voidNoArgs() = 0;
    virtual void voidVec3(Vec3 const &v) = 0;
    virtual void voidVec2Vec3(Vec2 const &a, Vec3 const &b) = 0;
    virtual void constMethod(int a) const = 0;
    virtual Vec3 const & vecRefNoArgs() const = 0;
    virtual ITest2 const & refComplexNoArgs() const = 0;
    virtual std::shared_ptr<ITest2> returnSharedPtr() = 0;
    virtual void sharedPtrArg(std::shared_ptr<ITest2> const &test2) const = 0;
};

}

DUK_CPP_DEF_CLASS_NAME(MethodTests::ITest2);

DUK_CPP_DEF_CLASS_NAME(MethodTests::Test2);
DUK_CPP_DEF_BASE_CLASS(MethodTests::Test2, MethodTests::ITest2);

DUK_CPP_DEF_CLASS_NAME(MethodTests::ITest);


template <class C, class R, class ... A>
static void registerMethod(duk::Context &d, const char *objName, const char *methodName, C *obj, R (C::*method)(A...)) {
    duk_push_global_object(d);

    auto testIdx = duk_push_object(d);

    duk_push_pointer(d, obj);
    duk_put_prop_string(d, testIdx, "\xff" "obj_ptr");

    duk::details::PushMethod(d, method);
    duk_put_prop_string(d, testIdx, methodName);

    duk_put_prop_string(d, -2, objName);
}

template <class C, class R, class ... A>
static void registerMethod(duk::Context &d, const char *objName, const char *methodName, C *obj, R (C::*method)(A...) const) {
    duk_push_global_object(d);

    auto testIdx = duk_push_object(d);

    duk_push_pointer(d, obj);
    duk_put_prop_string(d, testIdx, "\xff" "obj_ptr");

    duk::details::PushMethod(d, method);
    duk_put_prop_string(d, testIdx, methodName);

    duk_put_prop_string(d, -2, objName);
}

TEST_CASE("Method bindings tests", "[duktape]") {
    using namespace MethodTests;

    duk::Context d;

    SECTION("should handle const methods") {
        // arrange
        const char script[] = "test.constMethod(123);";
        Mock<ITest> mock;
        When(Method(mock, constMethod)).AlwaysReturn();
        ITest &test = mock.get();
        registerMethod(d, "test", "constMethod", &test, &ITest::constMethod);

        // act
        auto evalRes = duk_peval_string(d, script);
        if (evalRes != 0) {
            std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
        }

        // assert
        CHECK(evalRes == 0);
        bool isCalled = Verify(Method(mock, constMethod).Using(123)).Exactly(Once);
        REQUIRE(isCalled == 1);
    }

    SECTION("arguments") {
        SECTION("should bind methods with no arguments") {
            // arrange
            const char script[] = "test.intNoArgs();";
            Mock<ITest> mock;
            When(Method(mock, intNoArgs)).AlwaysReturn();
            ITest &test = mock.get();
            registerMethod(d, "test", "intNoArgs", &test, &ITest::intNoArgs);

            // act
            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
            }

            // assert
            CHECK(evalRes == 0);
            bool isCalled = Verify(Method(mock, intNoArgs)).Exactly(Once);
            REQUIRE(isCalled == 1);
        }

        SECTION("should bind methods with simple arguments") {
            // arrange
            const char script[] = "test.floatSimpleArgs(1.5, 3);";
            Mock<ITest> mock;
            When(Method(mock, floatSimpleArgs)).Return(4.5);
            ITest &test = mock.get();
            registerMethod(d, "test", "floatSimpleArgs", &test, &ITest::floatSimpleArgs);

            // act
            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
            }

            // assert
            CHECK(evalRes == 0);
            bool isCalled = Verify(Method(mock, floatSimpleArgs).Using(1.5, 3)).Exactly(Once);
            REQUIRE(isCalled == true);
        }

        SECTION("should handle arguments passed by reference") {
            // arrange
            const char script[] = "test.voidVec3({x: 3.5, y: 2.2, z: 1});";
            Mock<ITest> mock;
            bool argsMatched = false;
            When(Method(mock, voidVec3)).AlwaysDo([&argsMatched] (const Vec3 &v) {
                argsMatched = v == Vec3(3.5f, 2.2f, 1);
            });
            ITest &test = mock.get();
            registerMethod(d, "test", "voidVec3", &test, &ITest::voidVec3);

            // act
            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
            }

            // assert
            CHECK(evalRes == 0);
            bool isCalled = Verify(Method(mock, voidVec3)).Exactly(Once);
            REQUIRE(isCalled == true);
            REQUIRE(argsMatched == true);
        }

        SECTION("should handle multiple arguments") {
            // arrange
            const char script[] = "test.voidVec2Vec3({ x: 1, y: 2 }, { x: 3.5, y: 2.2, z: 1 });";
            Mock<ITest> mock;
            bool argsMatched = false;
            When(Method(mock, voidVec2Vec3)).AlwaysDo([&argsMatched] (const Vec2 &a, const Vec3 &b) {
                argsMatched = (a == Vec2(1, 2)) && (b == Vec3(3.5f, 2.2f, 1));
            });
            ITest &test = mock.get();
            registerMethod(d, "test", "voidVec2Vec3", &test, &ITest::voidVec2Vec3);

            // act
            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
            }

            // assert
            CHECK(evalRes == 0);
            bool isCalled = Verify(Method(mock, voidVec2Vec3)).Exactly(Once);
            REQUIRE(isCalled == true);
            REQUIRE(argsMatched == true);
        }

        SECTION("should handle methods with shared pointer arguments") {
            auto test2 = std::make_shared<ITest2>();
            Mock<ITest2> spy2(*test2);

            const char script[] = "test.sharedPtrArg(test2);";

            Mock<ITest> mock;
            bool argsMatched = false;
            When(Method(mock, sharedPtrArg)).Do([&argsMatched, test2] (std::shared_ptr<ITest2> const &arg) {
                argsMatched = arg == test2;
            });

            ITest &test = mock.get();
            registerMethod(d, "test", "sharedPtrArg", &test, &ITest::sharedPtrArg);
            d.addGlobal("test2", test2);

            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
            }
            REQUIRE(evalRes == 0);

            bool isCalled = Verify(Method(mock, sharedPtrArg)).Exactly(Once);
            REQUIRE(isCalled);
            REQUIRE(argsMatched);
        }
    }

    SECTION("return types") {
        SECTION("should bind methods with simple return types") {
            // arrange
            const char script[] = "var a = test.floatSimpleArgs(1.5, 3);";
            Mock<ITest> mock;
            When(Method(mock, floatSimpleArgs)).AlwaysReturn(4.5);
            ITest &test = mock.get();
            registerMethod(d, "test", "floatSimpleArgs", &test, &ITest::floatSimpleArgs);

            // act
            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
            }
            duk_push_global_object(d);
            duk_get_prop_string(d, -1, "a");
            float res = float(duk_get_number(d, -1));
            duk_pop_2(d);

            // assert
            CHECK(evalRes == 0);
            bool isCalled = Verify(Method(mock, floatSimpleArgs).Using(1.5, 3)).Exactly(Once);
            CHECK(isCalled == true);
            REQUIRE(res == 4.5);
        }

        SECTION("should bind methods with void return type") {
            // arrange
            const char script[] = "test.voidNoArgs();";
            Mock<ITest> mock;
            When(Method(mock, voidNoArgs)).AlwaysReturn();
            ITest &test = mock.get();
            registerMethod(d, "test", "voidNoArgs", &test, &ITest::voidNoArgs);

            // act
            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
            }

            // assert
            CHECK(evalRes == 0);
            bool isCalled = Verify(Method(mock, voidNoArgs)).Exactly(Once);
            REQUIRE(isCalled == true);
        }

        SECTION("should bind methods returning reference") {
            const char script[] = "test.vecRefNoArgs();";
            Mock<ITest> mock;
            Vec3 fakeRes(1.5f, 32.5f, 2.0f);

            When(Method(mock, vecRefNoArgs)).AlwaysDo([&fakeRes] () -> Vec3 const & {
                return fakeRes;
            });

            ITest &test = mock.get();
            registerMethod(d, "test", "vecRefNoArgs", &test, &ITest::vecRefNoArgs);

            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
            }
            REQUIRE(evalRes == 0);

            SECTION("method call happened") {
                bool called = Verify(Method(mock, vecRefNoArgs)).Exactly(Once);
                REQUIRE(called);
            }

            SECTION("result value converted") {
                Vec3 actualRes;
                duk::Type<Vec3>::get(d, actualRes, -1);
                REQUIRE(actualRes == fakeRes);
            }
        }

        SECTION("should bind methods with complex return type") {
            const char script[] =
                "test.refComplexNoArgs().someMethod(5374, {x: 31.5, y: -5.3, z: 6.9})";

            bool argsMatched;
            Mock<ITest2> mock2;
            When(Method(mock2, someMethod)).AlwaysDo([&argsMatched] (float number, Vec3 vec) -> Vec2 {
                argsMatched = (number == 5374) && (vec == Vec3(31.5f, -5.3f, 6.9f));
                return Vec2(59.1f, -23.2f);
            });
            ITest2 &test2 = mock2.get();

            Mock<ITest> mock1;
            When(Method(mock1, refComplexNoArgs)).AlwaysDo([&test2] () -> ITest2 const & {
                return test2;
            });
            ITest &test1 = mock1.get();

            registerMethod(d, "test", "refComplexNoArgs", &test1, &ITest::refComplexNoArgs);

            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
            }
            REQUIRE(evalRes == 0);

            SECTION("first object method called") {
                bool called1 = Verify(Method(mock1, refComplexNoArgs)).Exactly(Once);
                REQUIRE(called1);
            }

            SECTION("second object method called") {
                bool called2 = Verify(Method(mock2, someMethod)).Exactly(Once);
                REQUIRE(called2);

                SECTION("input arguments matched") {
                    REQUIRE(argsMatched);
                }

                SECTION("output argument matched") {
                    Vec2 expected = Vec2(59.1f, -23.2f);
                    Vec2 actual;
                    duk::Type<Vec2>::get(d, actual, -1.0f);
                    REQUIRE(actual == expected);
                }
            }
        }

        SECTION("should handle shared_ptr return types") {
            const char script[] =
                "var test2 = test.returnSharedPtr();\n"
                "test2.someMethod(5374, {x: 31.5, y: -5.3, z: 6.9});";

            bool argsMatched;
            std::shared_ptr<ITest2> test2 = std::make_shared<ITest2>();
            Mock<ITest2> mock2(*test2);
            When(Method(mock2, someMethod)).AlwaysDo([&argsMatched] (float number, Vec3 vec) -> Vec2 {
                argsMatched = (number == 5374) && (vec == Vec3(31.5f, -5.3f, 6.9f));
                return Vec2(59.1, -23.2);
            });

            Mock<ITest> mock1;
            When(Method(mock1, returnSharedPtr)).AlwaysDo([test2] () -> std::shared_ptr<ITest2> {
                return test2;
            });
            ITest &test1 = mock1.get();

            registerMethod(d, "test", "returnSharedPtr", &test1, &ITest::returnSharedPtr);

            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
            }
            REQUIRE(evalRes == 0);

            SECTION("first object method called") {
                bool called1 = Verify(Method(mock1, returnSharedPtr)).Exactly(Once);
                REQUIRE(called1);
            }

            SECTION("second object method called") {
                bool called2 = Verify(Method(mock2, someMethod)).Exactly(Once);
                REQUIRE(called2);

                SECTION("input arguments matched") {
                    REQUIRE(argsMatched);
                }

                SECTION("output argument matched") {
                    Vec2 expected = Vec2(59.1f, -23.2f);
                    Vec2 actual;
                    duk::Type<Vec2>::get(d, actual, -1);
                    REQUIRE(actual == expected);
                }
            }
        }
    }

    SECTION("polymorphic types") {
        SECTION("should handle polymorphic argument") {
            const char script[] = "test.sharedPtrArg(test2);";

            Mock<ITest> mock;
            When(Method(mock, sharedPtrArg)).AlwaysReturn();
            ITest &test = mock.get();

            auto test2 = std::make_shared<Test2>();

            d.addGlobal("test2", test2);
            registerMethod(d, "test", "sharedPtrArg", &test, &ITest::sharedPtrArg);

            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "error evaluating script: " << duk_safe_to_string(d, -1) << std::endl;
            }
            REQUIRE(evalRes == 0);

            bool called = Verify(Method(mock, sharedPtrArg)).Exactly(Once);
        }

        SECTION("should handle polymorphic return type") {
            const char script[] = "var test2 = test.returnSharedPtr();";
        }
    }
}
