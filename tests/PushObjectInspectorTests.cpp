#include <catch/catch.hpp>
#include <catch/fakeit.hpp>

#include <string>

#include <duktape-cpp/Context.inl>
#include <duktape-cpp/Types/All.h>
#include <duktape-cpp/PushObjectInspector.h>
#include <duktape-cpp/PushObjectInspector.inl>
#include <duktape-cpp/Utils/Inspect.h>

#include "TestTypes.h"

using namespace duk;

namespace PushObjectInspectorTests {

class Player {
public:
    Player(): _id(-1), _health(-1) {}
    Player(int id, float health): _id(id),  _health(health) {}

    int id() const { return _id; }

    float health() const { return _health; }
    void setHealth(float value) { _health = value; }

    Vec3 const & pos() const { return _pos; }
    void setPos(Vec3 const &v) { _pos = v; }

    void respawn(Vec3 const &pos, float health) {
        setPos(pos);
        setHealth(health);
    }

    std::string toString() const {
        return "id: {" + std::to_string(id()) + "}, health: {" + std::to_string(health()) + "}";
    }

    template <class Inspector>
    static void inspect(Inspector &i) {
        i.property("id", &Player::id);
        i.property("health", &Player::health, &Player::setHealth);
        i.property("pos", &Player::pos, &Player::setPos);
        i.method("respawn", &Player::respawn);
        i.method("toString", &Player::toString);
    }

private:
    int _id;
    float _health;
    Vec3 _pos;
};
}

TEST_CASE("PushObjectInspector tests", "[duktape]") {
    using namespace PushObjectInspectorTests;

    duk::Context d;

    SECTION("property") {
        Player p(135, 13.5);

        duk_push_global_object(d);
        auto pIdx = duk_push_object(d);
        duk_push_pointer(d, &p);
        duk_put_prop_string(d, pIdx, "\xff" "obj_ptr");
        duk::details::PushObjectInspector inspector(d, pIdx);
        Inspect<Player>::inspect(inspector);
        duk_put_prop_string(d, -2, "Player");
        duk_pop(d);

        SECTION("should bind property with only getter") {
            const char script[] = "Player.id;";

            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "script evaluation error: " << duk_safe_to_string(d, -1) << std::endl;
            }

            duk_push_context_dump(d);
            std::cout << duk_get_string(d, -1) << std::endl;
            duk_pop(d);

            int id = duk_get_int(d, -1);
            duk_pop(d);

            CHECK(evalRes == 0);
            REQUIRE(id == p.id());
        }

        SECTION("should bind property with getter and setter") {
            const char script[] = "var h = Player.health; Player.health = h - 10.5;";

            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "script evaluation error: " << duk_safe_to_string(d, -1) << std::endl;
            }

            CHECK(evalRes == 0);
            REQUIRE(p.health() == 13.5 - 10.5);
        }

        SECTION("object setter should work") {
            const char script[] = "Player.pos = { x: -5, y: -6, z: 7 };";

            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "script evaluation error: " << duk_safe_to_string(d, -1) << std::endl;
            }

            CHECK(evalRes == 0);
            REQUIRE(p.pos() == Vec3(-5, -6, 7));
        }
    }

    SECTION("methods") {
        Player p(135, 13.5);
        p.setPos(Vec3(10, 10, 10));

        duk_push_global_object(d);
        auto pIdx = duk_push_object(d);
        duk_push_pointer(d, &p);
        duk_put_prop_string(d, pIdx, "\xff" "obj_ptr");
        duk::details::PushObjectInspector inspector(d, pIdx);
        Inspect<Player>::inspect(inspector);
        duk_put_prop_string(d, -2, "Player");
        duk_pop(d);

        SECTION("should bind methods") {
            const char script[] = "Player.respawn({x: 5, y: 2.5, z: 3.5}, 56);";

            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "script evaluation error: " << duk_safe_to_string(d, -1) << std::endl;
            }

            CHECK(evalRes == 0);
            REQUIRE(p.pos() == Vec3(5.0f, 2.5f, 3.5f));
            REQUIRE(p.health() == 56);
        }

        SECTION("should bind const methods") {
            const char script[] = "Player.toString();";

            auto evalRes = duk_peval_string(d, script);
            if (evalRes != 0) {
                std::cout << "script evaluation error: " << duk_safe_to_string(d, -1) << std::endl;
            }
            REQUIRE(evalRes == 0);

            std::string resValue;
            duk::Type<std::string>::get(d, resValue, -1);
            duk_pop(d);

            REQUIRE(resValue == p.toString());
        }
    }

    SECTION("stack top should remain the same after inspect") {
        Player p;

        auto objIdx = duk_push_object(d);
        int topBefore = duk_get_top(d);

        duk::details::PushObjectInspector i(d, objIdx);
        Inspect<Player>::inspect(i);

        int topAfter = duk_get_top(d);

        REQUIRE(topBefore == topAfter);
    }
}
