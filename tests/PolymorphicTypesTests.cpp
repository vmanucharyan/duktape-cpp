#include <catch/catch.hpp>

#include <duktape-cpp/DuktapeCpp.h>

namespace PolymorphicTests {

class IBase {
public:
    virtual ~IBase() {}

    virtual int pureVirtualMethod() = 0;
    virtual std::string overriddenMethod() { return "base"; }
    int nonVirtualMethod() { return 100; }

    int refToBaseClassArg(IBase &base) { return base.pureVirtualMethod(); }

    template <class Inspector>
    static void inspect(Inspector &i) {
        i.method("pureVirtualMethod", &IBase::pureVirtualMethod);
        i.method("overriddenMethod", &IBase::overriddenMethod);
        i.method("nonVirtualMethod", &IBase::nonVirtualMethod);
        i.method("refToBaseClassArg", &IBase::refToBaseClassArg);
    }
};

class Concrete: public IBase {
public:
    explicit Concrete(int someProp) : _someProp(someProp) {}

    int pureVirtualMethod() override {
        return _someProp;
    }

    std::string overriddenMethod() override {
        return "concrete";
    }

    int someProp() const {
        return _someProp;
    }

    void setSomeProp(int someProp) {
        _someProp = someProp;
    }

    void refToConcreteClassArg(Concrete &other) {
        other.setSomeProp(this->someProp());
    }

    int ptToBaseClassArg(std::shared_ptr<IBase> other) {
        return other->pureVirtualMethod();
    }

    template <class Inspector>
    static void inspect(Inspector &i) {
        i.construct(&std::make_shared<Concrete, int>);
        i.property("someProp", &Concrete::someProp, &Concrete::setSomeProp);
        i.method("refToConcreteClassArg", &Concrete::refToConcreteClassArg);
        i.method("ptToBaseClassArg", &Concrete::ptToBaseClassArg);
    }

private:
    int _someProp { 0 };
};

class SubConcrete: public Concrete {
public:
    explicit SubConcrete(int someProp) : Concrete(someProp) {}

    template <class Inspector>
    static void inspect(Inspector &i) {
        i.construct(&std::make_shared<SubConcrete, int>);
    }
};

}

DUK_CPP_DEF_CLASS_NAME(PolymorphicTests::Concrete);
DUK_CPP_DEF_BASE_CLASS(PolymorphicTests::Concrete, PolymorphicTests::IBase);

DUK_CPP_DEF_CLASS_NAME(PolymorphicTests::SubConcrete);
DUK_CPP_DEF_BASE_CLASS(PolymorphicTests::SubConcrete, PolymorphicTests::Concrete);

TEST_CASE("Polymorphic classes", "[duktape-cpp]") {
    using PolymorphicTests::IBase;
    using PolymorphicTests::Concrete;
    using PolymorphicTests::SubConcrete;

    duk::Context ctx;
    ctx.registerClass<Concrete>();
    ctx.registerClass<SubConcrete>();

    SECTION("when created from script") {
        SECTION("should be able to get pointer to base class") {
            std::shared_ptr<IBase> basePtr;
            ctx.evalString(basePtr, "new PolymorphicTests.Concrete(1234)");

            REQUIRE(basePtr != nullptr);
            REQUIRE(basePtr->pureVirtualMethod() == 1234);
        }

        SECTION("should be able to get pointer to concrete class") {
            std::shared_ptr<Concrete> concretePtr;
            ctx.evalString(concretePtr, "new PolymorphicTests.Concrete(4321)");

            REQUIRE(concretePtr != nullptr);
            REQUIRE(concretePtr->someProp() == 4321);
        }

        SECTION("should bind base class methods") {
            std::tuple<int, std::string, int, int> res;
            ctx.evalString(res,
                "var obj = new PolymorphicTests.Concrete(123);\n"
                "var a = obj.pureVirtualMethod();\n"
                "var b = obj.overriddenMethod();\n"
                "var c = obj.nonVirtualMethod();\n"
                "var d = obj.someProp;\n"
                "[a, b, c, d]"
            );

            auto expected = std::make_tuple(123, "concrete", 100, 123);

            REQUIRE(res == expected);
        }

        SECTION("should be able to push shared pointer to base class ") {
            std::shared_ptr<IBase> basePtr = std::make_shared<Concrete>(1233);
            ctx.addGlobal("obj", basePtr);

            int res = -1;
            ctx.evalString(res, "obj.pureVirtualMethod()");

            REQUIRE(res == 1233);
        }

        SECTION("call method that accepts reference to base class") {
            int res = -1;
            ctx.evalString(res,
                "var a = new PolymorphicTests.Concrete(123);\n"
                "var b = new PolymorphicTests.Concrete(456);\n"
                "b.refToBaseClassArg(a)"
            );

            REQUIRE(res == 123);
        }

        SECTION("call method that accepts reference to concrete class") {
            int res = -1;
            ctx.evalString(res,
                "var a = new PolymorphicTests.Concrete(123);\n"
                "var b = new PolymorphicTests.Concrete(456);\n"
                "b.refToConcreteClassArg(a);\n"
                "a.someProp"
            );

            REQUIRE(res == 456);
        }

        SECTION("call method that accepts reference to subbase class") {
            int res = -1;
            ctx.evalString(res,
                "var a = new PolymorphicTests.SubConcrete(123);\n"
                "var b = new PolymorphicTests.SubConcrete(456);\n"
                "b.refToBaseClassArg(a)"
            );

            REQUIRE(res == 123);
        }

        SECTION("call method that accepts reference to subconcrete class") {
            int res = -1;
            ctx.evalString(res,
                "var a = new PolymorphicTests.SubConcrete(123);\n"
                "var b = new PolymorphicTests.SubConcrete(456);\n"
                "b.refToConcreteClassArg(a);\n"
                "a.someProp"
            );

            REQUIRE(res == 456);
        }
    }
}
