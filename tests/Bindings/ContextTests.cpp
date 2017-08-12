#include <catch/catch.hpp>
#include <catch/fakeit.hpp>

#include <Engine/EngineStd.h>

#include <Engine/Game/Actors/Actor.h>
#include <Engine/Game/Actors/ActorComponent.h>

#include <Engine/Game/Events/EventBase.h>
#include <Engine/Game/Events/EventManager.h>

#include <Engine/Duktape/Bindings/Prelude.h>

using namespace engine;

namespace ContextTests {

class Player {
public:
    static std::shared_ptr<Player> CreatePlayer(int id) {
        return makeShared<Player>(id);
    }

    Player(): _id(0) {}

    Player(int id): _id(id) {}

    template <class Inspector>
    static void inspect(Inspector &i) {
        i.construct(&Player::CreatePlayer);
        i.property("id", &Player::id, &Player::setId);
    }

    int id() const { return this->_id; }
    void setId(int id) { this->_id = id; }

    int _id;
};

class PositionComponent: public ActorComponent<PositionComponent> {
public:
    static sp<PositionComponent> Construct(Vec3 const &pos) {
        return makeShared<PositionComponent>(pos);
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

class DeathEvent: public Event<DeathEvent> {
public:
    static up<DeathEvent> Construct(std::string const &actorId) {
        return makeUnique<DeathEvent>(actorId);
    }

    DeathEvent(std::string const &actorId): _actorId(actorId) {}

    std::string const & actorId() const { return _actorId; }

    template <class I>
    static void inspect(I &i) {
        i.construct(&DeathEvent::Construct);
        i.property("actorId", &DeathEvent::actorId);
    }

private:
    std::string _actorId;
};

enum class TestEnum {
    VALUE_1 = 1,
    VALUE_2 = 2
};

}

namespace engine {

template <>
struct Inspect<ContextTests::TestEnum> {
    template <class I>
    static void inspect(I &i) {
        i.constant("VALUE_1", ContextTests::TestEnum::VALUE_1);
        i.constant("VALUE_2", ContextTests::TestEnum::VALUE_2);
    }
};

namespace duk {

template <>
struct Type<ContextTests::TestEnum> {
    static void push(duk::Context &d, ContextTests::TestEnum const &val) {
        duk_push_int(d, int(val));
    }

    static void get(duk::Context &d, ContextTests::TestEnum &val, int index) {
        val = ContextTests::TestEnum(duk_require_int(d, index));
    }
};

}
}

DEF_CLASS_NAME(ContextTests::Player);
DEF_CLASS_NAME(ContextTests::PositionComponent);
DEF_CLASS_NAME(ContextTests::DeathEvent);
DEF_CLASS_NAME(ContextTests::TestEnum);

TEST_CASE("Context", "[duktape]") {
    using namespace ContextTests;

    duk::Context ctx;

    SECTION("registerClass") {
        SECTION("should register class constructor") {
            const char script[] = "new ContextTests.Player(9)";

            ctx.registerClass<ContextTests::Player>();

            duk_eval_string(ctx, script);

            sp<ContextTests::Player> popped;
            duk::Type<sp<ContextTests::Player>>::get(ctx,  popped, -1);
            duk_pop(ctx);

            REQUIRE(popped->id() == 9);

            SECTION("should be able to access class fields from script") {
                const char sc[] = "var a = new ContextTests.Player(9);\n"
                                  "a.id";

                duk_eval_string(ctx, sc);
                int id = duk_get_int(ctx, -1);
                duk_pop(ctx);

                REQUIRE(id == 9);
            }

            SECTION("should not corrupt stack") {
                REQUIRE(duk_get_top(ctx) == 0);
            }
        }
    }

    SECTION("addGlobal") {
        duk::Context d;

        auto ev = makeShared<EventManager>();
        auto am = makeShared<ActorsManager>(ev);

        d.addGlobal("Actors", am);

        SECTION("should push shared ptr objects") {
            auto player = makeShared<Actor>("player");
            am->addActor(player);
            const char script[] = "Actors.getActor('player')";

            duk_eval_string(d, script);

            sp<Actor> stackActor;
            duk::Type<sp<Actor>>::get(d, stackActor, -1);

            REQUIRE(stackActor == player);
        }
    }

    SECTION("registerComponent") {
        duk::Context d;

        auto ev = makeShared<EventManager>();
        auto am = makeShared<ActorsManager>(ev);

        d.addGlobal("Actors", am);
        d.registerComponent<PositionComponent>();

        SECTION("should register function in js that gets component from actor") {
            auto player = makeShared<Actor>("player");
            player->attachComponent<PositionComponent>(Vec3(1, 2, 3));

            am->addActor(player);
            const char script[] =
                "var actor = Actors.getActor('player')\n"
                "var pc = ContextTests.PositionComponent(actor)\n"
                "pc.pos = { x: 3, y: 4, z: 5.5 }\n";

            d.evalStringNoRes(script);

            REQUIRE(player->getComponent<PositionComponent>()->pos() == Vec3(3, 4, 5.5f));

            SECTION("component should have detach method") {
                const char scriptDetach[] =
                    "var actor = Actors.getActor('player')\n"
                    "ContextTests.PositionComponent(actor).detach()\n";

                d.evalStringNoRes(scriptDetach);

                REQUIRE(player->getComponent<PositionComponent>() == nullptr);
            }
        }

        SECTION("should register function in js that makes component") {
            const char script[] =
                "var actor = Actors.makeActor('player');"
                "var pc = new ContextTests.PositionComponent(actor, { x: 5, y: 4, z: 3 });";

            d.evalStringNoRes(script);

            auto player = am->getActor("player");
            auto pc = player->getComponent<PositionComponent>();

            REQUIRE(pc->pos() == Vec3(5.0f, 4.0f, 3.0f));
        }
    }

    SECTION("evalString") {
        duk::Context d;

        const char script[] = "5 + 10";
        int expectedResult = 15;

        int actualResult = -1;
        d.evalString(actualResult, script);

        SECTION("should evaluate script and return result") {
            REQUIRE(actualResult == expectedResult);
        }

        SECTION("should not pollute stack") {
            REQUIRE(duk_get_top(d) == 0);
        }
    }

    SECTION("registerEvent") {
        duk::Context d;

        SECTION("should register event class constructor") {
            // arrange
            const char script[] = "new ContextTests.DeathEvent('player1')";
            d.registerEvent<DeathEvent>();

            // act
            up<DeathEvent> evalRes;
            d.evalString(evalRes, script);

            // assert
            REQUIRE(evalRes->actorId() == "player1");
        }

        SECTION("should register nested `Delegate` class") {
            // arrange
            const char script[] = "new ContextTests.DeathEvent.Delegate(function () { })";
            d.registerEvent<DeathEvent>();

            // act
            sp<DeathEvent::Delegate> evalRes;
            d.evalString(evalRes, script);

            // assert
            REQUIRE(evalRes != nullptr);
        }

        SECTION("should not pollute stack") {
            // act
            d.registerEvent<DeathEvent>();

            // assert
            REQUIRE(duk_get_top(d) == 0);
        }
    }

    SECTION("registerEnum") {
        duk::Context d;

        d.registerEnum<ContextTests::TestEnum>();

        SECTION("should register enum in javascript") {
            const char testScript[] = "ContextTests.TestEnum.VALUE_1";
            ContextTests::TestEnum expected = ContextTests::TestEnum::VALUE_1;

            ContextTests::TestEnum actual;
            d.evalString(actual, testScript);

            REQUIRE(actual == expected);
        }

        SECTION("should not pollute stack") {
            REQUIRE(duk_get_top(d) == 0);
        }
    }

    SECTION("stashing") {
        duk::Context d;
        d.evalStringNoRes("f1 = function() { return 1 }; f2 = function() { return 2 }; o = { a: 'b' }; ");

        duk_get_global_string(d, "f2");
        int f2Key = d.stashRef(-1);
        duk_pop(d);

        duk_get_global_string(d, "f1");
        int f1Key = d.stashRef(-1);
        duk_pop(d);

        duk_get_global_string(d, "o");
        int oKey = d.stashRef(-1);
        duk_pop(d);

        SECTION("stashRef") {
            SECTION("should store object reference in stash.refs") {
                duk_push_global_stash(d);
                duk_get_prop_string(d, -1, "refs");
                REQUIRE(duk_has_prop_index(d, -1, f2Key));

                // call stored function to make sure we stored the right object
                duk_get_prop_index(d, -1, (duk_uarridx_t) f2Key);
                REQUIRE(duk_is_callable(d, -1));

                duk_call(d, 0);

                int res;
                duk::Type<int>::get(d, res, -1);
                REQUIRE(res == 2);
            }

            SECTION("should not pollute stash") {
                REQUIRE(duk_get_top(d) == 0);
            }
        }

        SECTION("unstashRef") {
            d.unstashRef(f2Key);

            SECTION("should delete object from stash.refs") {
                duk_push_global_stash(d);
                duk_get_prop_string(d, -1, "refs");
                REQUIRE_FALSE(duk_has_prop_index(d, -1, f2Key));
            }

            SECTION("should not pollute stash") {
                REQUIRE(duk_get_top(d) == 0);
            }
        }

        SECTION("getRef") {
            SECTION("should place stored object on top of the stack") {
                d.getRef(f1Key);
                REQUIRE(duk_is_callable(d, -1));

                duk_call(d, 0);
                int res;
                duk::Type<int>::get(d, res, -1);

                REQUIRE(res == 1);
            }

            SECTION("should not pollute stash") {
                REQUIRE(duk_get_top(d) == 0);
            }
        }
    }

    SECTION("evalStringNoRes") {
        duk::Context d;
        SECTION("should clear stack from return value") {
            d.evalStringNoRes("5 + 5");
            REQUIRE(duk_get_top(d) == 0);
        }
    }
}
