#include <memory>
#include <iostream>

#include <duktape-cpp/DuktapeCpp.h>

namespace SpaceInvaders {

class Spaceship {
public:
    explicit Spaceship(int pos) : _pos(pos) {}

    void moveLeft() { _pos -= 1; }
    void moveRight() { _pos += 1; }

    int pos() const { return _pos; }

    /**
     * Inspect method defines class meta information (methods, properties etc..)
     * You can define `inspect` method inline or specialize `duk::Inspect` for your class
     */
    template <class Inspector>
    static void inspect(Inspector &i) {
        i.construct(&std::make_shared<Spaceship, int>);
        i.method("moveRight", &Spaceship::moveRight);
        i.method("moveLeft", &Spaceship::moveLeft);
        i.property("pos", &Spaceship::pos);
    }

private:
    int _pos;
};

}

DUK_CPP_DEF_CLASS_NAME(SpaceInvaders::Spaceship);

int main(int argc, char **argv) {
    try {
        /**
         * Create context
         */
        duk::Context ctx;

        /**
         * Register class.
         * Make sure the following requirements are met:
         * - Either `inspect` method or `Inspect` template specialization must be defined
         * - Class name must be defined (via DEF_CLASS_NAME macro)
         */
        ctx.registerClass<SpaceInvaders::Spaceship>();

        /**
         * Create spaceship in js
         */
        ctx.evalStringNoRes("var spaceship = new SpaceInvaders.Spaceship(5)");

        /**
         * Get shared pointer to spaceship
         */
        std::shared_ptr<SpaceInvaders::Spaceship> spaceship;
        ctx.getGlobal("spaceship", spaceship);
        assert(spaceship);
        assert(spaceship->pos() == 5);

        ctx.evalStringNoRes("spaceship.moveRight()");
        assert(spaceship->pos() == 6);

        spaceship->moveLeft();

        /**
         * Evaluate script and get result
         */
        int spaceshipPos = -1;
        ctx.evalString(spaceshipPos, "spaceship.pos");
        assert(spaceshipPos == 5);
    }
    catch(duk::ScriptEvaluationExcepton &e) {
        std::cout << e.what() << std::endl;
    }
}