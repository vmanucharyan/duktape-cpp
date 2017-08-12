#include <catch/catch.hpp>
#include <catch/fakeit.hpp>

#include <Engine/EngineStd.h>

#include <Engine/Game/Actors/Actor.h>
#include <Engine/Game/Actors/ActorComponent.h>

#include <Engine/Game/Events/EventManager.h>

#include <Engine/Game/CommonComponents/TransformComponent.h>

#include <Engine/Sound/Components/SoundListenerComponent.h>

#include <Engine/Physics/CollisionEvent.h>

#include <Engine/Duktape/Bindings/Prelude.h>

using namespace engine;

TEST_CASE("Engine integration tests", "[duktape]") {
    duk::Context d;

    auto ev = makeShared<EventManager>();
    auto am = makeShared<ActorsManager>(ev);

    d.addGlobal("Actors", am);

    d.registerComponent<TransformComponent>();
    d.registerComponent<SoundListenerComponent>();

    auto player = am->makeActor("player");
    player->attachComponent<TransformComponent>(Transforms(Vec3(0.0f, 5.0f, 2.5f)));

    SECTION("actors") {
        SECTION("should be able to create new actors") {
            const char script[] = "Actors.makeActor('player2');";

            d.evalStringNoRes(script);

            auto player2 = am->getActor("player2");

            REQUIRE(player2 != nullptr);
        }

        SECTION("should be able to query actors by components") {
            const char script[] =
                "Actors.getActorsWithComponents([engine.TransformComponent.Type])";
            std::vector<sp<Actor>> expected = { player };

            std::vector<sp<Actor>> actual;
            d.evalString(actual, script);

            REQUIRE(actual == expected);
        }

        SECTION("should be able to query actors by id") {
            const char script[] =
                "Actors.getActor('player')";
            sp<Actor> expected = player;

            sp<Actor> actual;
            d.evalString(actual, script);

            REQUIRE(actual == expected);
        }
    }

    SECTION("components") {
        SECTION("should be able to get actors component") {
            const char script[] =
                "var player = Actors.getActor('player');"
                "engine.TransformComponent(player)";
            auto expected = player->getComponent<TransformComponent>();

            sp<TransformComponent> actual;
            d.evalString(actual, script);

            REQUIRE(expected == actual);
        }

        SECTION("should be able to attach component to actor") {
            const char script[] =
                "var player = Actors.getActor('player');"
                "new engine.SoundListenerComponent(player, 5.5);";

            float expectedVolume = 5.5f;

            d.evalStringNoRes(script);

            REQUIRE(player->getComponent<SoundListenerComponent>() != nullptr);
            REQUIRE(player->getComponent<SoundListenerComponent>()->volume() == 5.5f);
        }

        SECTION("should be able to detach component from actor") {
            const char script[] =
                "var player = Actors.getActor('player');"
                "engine.TransformComponent(player).detach();";

            d.evalStringNoRes(script);

            REQUIRE(player->getComponent<TransformComponent>() == nullptr);
        }

        SECTION("should be able to get component type") {
            const char script[] =
                "engine.SoundListenerComponent.Type";
            const char *expected = engine::GetClassName<SoundListenerComponent>();

            std::string actual;
            d.evalString(actual, script);

            REQUIRE(actual == std::string(expected));
        }
    }

    d.addGlobal("Events", ev);
    d.registerEvent<CollisionEvent>();

    SECTION("events") {
        SECTION("should be able to trigger events") {
            // arrange
            const char script[] =
                "var actor1 = Actors.makeActor('a1');"
                "var actor2 = Actors.makeActor('a2');"
                "var event = new engine.CollisionEvent(actor1.id, actor2.id);"
                "Events.fireEvent(event);";

            bool eventTriggered = false;
            std::string actorA = "";
            std::string actorB = "";

            auto delegate = makeShared<CollisionEvent::Delegate>(
            [&eventTriggered, &actorA, &actorB] (CollisionEvent const &e) {
                eventTriggered = true;
                actorA = e.actorA();
                actorB = e.actorB();
            });
            ev->addListener(delegate);

            // act
            d.evalStringNoRes(script);

            ev->update(0);

            // assert
            REQUIRE(eventTriggered == true);
            REQUIRE(actorA == "a1");
            REQUIRE(actorB == "a2");
        }

        SECTION("should be able to subscribe to events in js") {
            // arrange
            const char script[] =
                "var actorA = '';"
                "var actorB = '';"
                "function onCollisionEvent(e) {"
                "  actorA = e.actorA;"
                "  actorB = e.actorB;"
                "}"
                "var collisionEventDelegate = new engine.CollisionEvent.Delegate(onCollisionEvent);"
                "Events.addListener(collisionEventDelegate);";

            std::string expectedActorA = "a1";
            std::string expectedActorB = "a2";
            
            // act
            d.evalStringNoRes(script);
            ev->fireEvent(makeUnique<CollisionEvent>(expectedActorA, expectedActorB));
            ev->update(0);

            duk_push_global_object(d);

            duk_get_prop_string(d, -1, "actorA");
            std::string actorA;
            duk::Type<std::string>::get(d, actorA, -1);
            duk_pop(d);

            duk_get_prop_string(d, -1, "actorB");
            std::string actorB;
            duk::Type<std::string>::get(d, actorB, -1);
            duk_pop(d);

            // assert
            REQUIRE(actorA == expectedActorA);
            REQUIRE(actorB == expectedActorB);
        }
    }
}
