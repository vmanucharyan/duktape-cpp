#include <catch/catch.hpp>

#include <cstdio>

#include <duktape-cpp/Context.inl>

#include <duktape-cpp/Types/All.h>
#include <duktape-cpp/PushObjectInspector.inl>

#include "TestTypes.h"

using namespace duk;

namespace SharedPtrTests {

class Player {
public:
    Player(int id, float health, Vec3 pos): _id(id),  _health(health), _pos(pos) {}

    int id() const { return _id; }

    float health() const { return _health; }
    void setHealth(float value) { _health = value; }

    Vec3 const & pos() const { return _pos; }
    void setPos(Vec3 const &v) { _pos = v; }

    void respawn(Vec3 const &pos, float health) {
        setPos(pos);
        setHealth(health);
    }

    template <class Inspector>
    static void inspect(Inspector &i) {
        i.property("id", &Player::id);
        i.property("health", &Player::health, &Player::setHealth);
        i.property("pos", &Player::pos, &Player::setPos);
        i.method("respawn", &Player::respawn);
    }

    bool operator== (Player const &other) const {
        return  other._health == this->_health &&
                other._pos == this->_pos &&
                other._id == this->_id;
    }

private:
    int _id;
    float _health;
    Vec3 _pos;
};

class Base {
public:
    virtual void setField(Vec3 field) = 0;
    virtual Vec3 getField() const = 0;

    template <class I>
    static void inspect(I &i) {
        i.method("getField", &Base::getField);
        i.method("setField", &Base::setField);
    }
};

class Concrete: public Base {
public:
    Concrete(Vec3 field): _someField(field) {}

    Vec3 getField() const override {
        return _someField;
    }

    void setField(Vec3 field) override {
        _someField = field;
    }

    void consume(std::unique_ptr<Concrete> other) {
        std::unique_ptr<Concrete> ptr = std::move(other);
    }

    template <class I>
    static void inspect(I &i) {
        i.method("getField", &Base::getField);
        i.method("setField", &Base::setField);
    }

private:
    Vec3 _someField;
};

}

DUK_CPP_DEF_BASE_CLASS(SharedPtrTests::Concrete, SharedPtrTests::Base);

TEST_CASE("Shared pointer type tests", "[duktape]") {
    using namespace SharedPtrTests;

    duk::Context ctx;

    SECTION("should push shared pointer to inspectable object") {
        const char script[] =
            "player.health = 33;"
            "player.pos = {x: 14.5, y: -13.5, z: 3.3};"
            "player.health;";

        auto player = std::make_shared<Player>(14, 87, Vec3(54, 12, 43));
        ctx.addGlobal("player", player);

        auto evalRes = duk_peval_string(ctx, script);
        if (evalRes != 0) {
            printf("script eval error: %s", duk_get_string(ctx, -1));
        }
        REQUIRE(evalRes == 0);

        // get result
        float health = float(duk_get_number(ctx, -1));
        duk_pop(ctx);

        REQUIRE(health == player->health());
        REQUIRE(health == 33);
        REQUIRE(player->pos() == Vec3(14.5, -13.5, 3.3));
    }

    SECTION("should get shared pointer of inspectable object from stack") {
        const char script[] = "player";

        auto player = std::make_shared<Player>(14, 87, Vec3(54, 12, 43));
        ctx.addGlobal("player", player);

        auto evalRes = duk_peval_string(ctx, script);
        if (evalRes != 0) {
            printf("script eval error: %s", duk_get_string(ctx, -1));
        }
        REQUIRE(evalRes == 0);

        std::shared_ptr<Player> p;
        duk::Type<std::shared_ptr<Player>>::get(ctx, p, -1);
        duk_pop(ctx);

        REQUIRE(p == player);
    }

    SECTION("should be able to get pointer to polymorphic object") {
        auto concrete = std::make_shared<Concrete>(Vec3(32.5f, -13.5f, 34.0f));
        ctx.addGlobal("concrete", concrete);

        auto evalRes = duk_peval_string(ctx, "concrete");
        if (evalRes != 0) {
            printf("script eval error: %s", duk_get_string(ctx, -1));
        }
        REQUIRE(evalRes == 0);

        std::shared_ptr<Concrete> actual;
        duk::Type<std::shared_ptr<Concrete>>::get(ctx, actual, -1);

        REQUIRE(actual->getField() == Vec3(32.5f, -13.5f, 34.0f));
    }

    SECTION("should be able to get object as shared pointer to it's base class") {
        auto concrete = std::make_shared<Concrete>(Vec3(32.5f, -13.5f, 34.0f));
        ctx.addGlobal("concrete", concrete);

        auto evalRes = duk_peval_string(ctx, "concrete");
        if (evalRes != 0) {
            printf("script eval error: %s", duk_get_string(ctx, -1));
        }
        REQUIRE(evalRes == 0);

        std::shared_ptr<Base> base;
        duk::Type<std::shared_ptr<Base>>::get(ctx, base, -1);

        REQUIRE(base->getField() == Vec3(32.5, -13.5, 34));
    }

    SECTION("should be able to to push shared pointer to base class") {
        std::shared_ptr<Base> base = std::make_shared<Concrete>(Vec3(32.5f, -13.5f, 34.0f));

        ctx.addGlobal("base", base);

        auto evalRes = duk_peval_string(ctx, "var pt = {x: -5, y: -6, z: -7}; base.setField(pt);");
        if (evalRes != 0) {
            printf("script eval error: %s", duk_get_string(ctx, -1));
        }
        REQUIRE(evalRes == 0);

        REQUIRE(base->getField() == Vec3(-5, -6, -7));
    }
}

TEST_CASE("Unique ptr tests", "[duktape]") {
    using namespace SharedPtrTests;

    duk::Context ctx;

    SECTION("should push unique pointer to inspectable object") {
        const char script[] =
            "player.health = 33;"
            "player.pos = {x: 14.5, y: -13.5, z: 3.3};"
            "player.health";

        auto player = std::make_unique<Player>(14, 87, Vec3(54.0f, 12.0f, 43.0f));

        Player * playerPtr = player.get();

        ctx.addGlobal("player", std::move(player));

        auto evalRes = duk_peval_string(ctx, script);
        if (evalRes != 0) {
            printf("script eval error: %s", duk_get_string(ctx, -1));
        }
        REQUIRE(evalRes == 0);

        // get result
        float health = float(duk_get_number(ctx, -1));
        duk_pop(ctx);

        REQUIRE(health == playerPtr->health());
        REQUIRE(health == 33);
        REQUIRE(playerPtr->pos() == Vec3(14.5, -13.5, 3.3));
    }

    SECTION("should be able to push and get unique_ptr") {
        auto player = std::make_unique<Player>(14, 87, Vec3(54.0f, 12.0f, 43.0f));
        auto expectedPlayer = *player;

        duk::Type<std::unique_ptr<Player>>::push(ctx, std::move(player));

        std::unique_ptr<Player> actualPlayer;
        duk::Type<std::unique_ptr<Player>>::get(ctx, actualPlayer, -1);

        REQUIRE(*actualPlayer == expectedPlayer);
    }

    SECTION("should be able to get object as unique pointer to it's base class") {
        auto concrete = std::make_unique<Concrete>(Vec3(32.5f, -13.5f, 34.0f));
        ctx.addGlobal("concrete", std::move(concrete));

        auto evalRes = duk_peval_string(ctx, "concrete");
        if (evalRes != 0) {
            printf("script eval error: %s", duk_get_string(ctx, -1));
        }
        REQUIRE(evalRes == 0);

        std::unique_ptr<Base> base;
        duk::Type<std::unique_ptr<Base>>::get(ctx, base, -1);

        REQUIRE(base->getField() == Vec3(32.5f, -13.5f, 34.0f));
    }

    SECTION("should be able to get pointer to polymorphic object") {
        auto concrete = std::make_unique<Concrete>(Vec3(32.5f, -13.5f, 34.0f));
        ctx.addGlobal("concrete", std::move(concrete));

        auto evalRes = duk_peval_string(ctx, "concrete");
        if (evalRes != 0) {
            printf("script eval error: %s", duk_get_string(ctx, -1));
        }
        REQUIRE(evalRes == 0);

        std::unique_ptr<Concrete> actual;
        duk::Type<std::unique_ptr<Concrete>>::get(ctx, actual, -1);

        REQUIRE(actual->getField() == Vec3(32.5f, -13.5f, 34.0f));
    }

    SECTION("should be able to to push unique pointer to base class") {
        auto base = std::make_unique<Concrete>(Vec3(32.5f, -13.5f, 34));
        Base* basePtr = base.get();

        ctx.addGlobal("base", std::move(base));

        auto evalRes = duk_peval_string(ctx, "var pt = {x: -5, y: -6, z: -7}; base.setField(pt);");
        if (evalRes != 0) {
            printf("script eval error: %s", duk_get_string(ctx, -1));
        }
        REQUIRE(evalRes == 0);

        REQUIRE(basePtr->getField() == Vec3(-5, -6, -7));
    }
}
