#include <catch/catch.hpp>
#include <catch/fakeit.hpp>

#include <math/Vec3.h>

#include <Engine/Game/Actors/Actor.h>
#include <Engine/Game/Events/EventManager.h>
#include <Engine/Game/Actors/ActorsManager.h>
#include <Engine/Game/Actors/ActorComponent.h>

#include <Engine/Duktape/Bindings/Prelude.h>

using namespace engine;

namespace ComponentTests {

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

}


DEF_CLASS_NAME(ComponentTests::PositionComponent);

TEST_CASE("component bindings test", "[duktape]") {
    using namespace ComponentTests;

    duk::Context d;
    // d.registerClass<Actor>();

    auto ev = makeShared<EventManager>();
    auto am = makeShared<ActorsManager>(ev);

    d.addGlobal("Actors", am);

    SECTION("Component<T>::registerComponent function") {
        duk_push_global_object(d);
        duk::details::Component<PositionComponent>::registerComponent(d, "PositionComponent", -1);
        duk_pop(d);

        SECTION("should register function in js that gets component") {
            auto player = makeShared<Actor>("player");
            player->attachComponent<PositionComponent>(Vec3(1, 2, 3));

            am->addActor(player);
            const char script[] =
                "var actor = Actors.getActor('player');\n"
                "var pc = PositionComponent(actor)\n;"
                "pc.pos = { x: 3, y: 4, z: 5 };";

            d.evalStringNoRes(script);

            REQUIRE(player->getComponent<PositionComponent>()->pos() == Vec3(3, 4, 5));
        }
    }
}
