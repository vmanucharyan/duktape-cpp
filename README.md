# Summary

`duktape-cpp` is a library to easily bind C++ classes to duktape scripts.

It is work-in-progress and may lack some features you may need, so feel free to contribute!

# Getting started

duktape-cpp is a header-only library, so all you need is to just add source
files from `src` directory to you project and `#include "duktape-cpp/DuktapeCpp.h"`

# Basic example

```cpp
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
     * You can define `inspect` method or specialize `duk::Inspect` for your class
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
         * - Class name must be defined (via DUK_CPP_DEF_CLASS_NAME macro)
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
```

# Usage

## Creating duktape context

First of all, we need to create an instance of `duk::Context`.

```cpp
duk::Context ctx;
```

It is basically a wrapper around `duk_context`.

## Defining inspectors

First, we need to tell `duktape-cpp` which members of class need to be exposed.
This can be done by defining `inspect` method inside of out class.

```cpp
namespace SpaceInvaders {

class Spaceship {
public:
    explicit Spaceship(int pos) : _pos(pos) {}

    void moveLeft() { _pos -= 1; }
    void moveRight() { _pos += 1; }

    int pos() const { return _pos; }
    
    template <class Inspector>
    static void inspect(Inspector &i) {
        i.method("moveRight", &Spaceship::moveRight);
        i.method("moveLeft", &Spaceship::moveLeft);
        i.property("pos", &Spaceship::pos);
    }

private:
    int _pos;
};

}
```

Another way of doing this, is by specializing `Inspector` template
(for example if we want to expose class from 3rd party library):

```cpp
namespace duk {

template<>
struct Inspect<SpaceInvaders::Spaceship> {
    template <class Inspector>
    static void inspect(Inspector &i) {
        i.method("moveRight", &Spaceship::moveRight);
        i.method("moveLeft", &Spaceship::moveLeft);
        i.property("pos", &Spaceship::pos);
    }
};

}
```

Note, that specialization must be in `duk` namespace

## Defining class name

Then, we need to specify class name. We can do it with `DUK_CPP_DEF_CLASS_NAME` macro

```
DUK_CPP_DEF_CLASS_NAME(SpaceInvaders::Spaceship)
```

Full name will be translated into `SpaceInvaders.Spaceship`.

We can specify another, shorter name by using `DUK_CPP_SEF_SHORT_NAME` macro

```
DUK_CPP_DEF_CLASS_NAME(SpaceInvaders::Spaceship, "Spaceship")
```

## Constructors

If we want to be able to create a spaceship from javascript code,
we have to define a constructor:

```
template <class Inspector>
static void inspect(Inspector &i) {
    i.construct(&std::make_shared<SpaceInvaders::Spaceship>);
}
```

As a constructor, we specify a function that returns shared pointer
to our class. In this example, we simply use `std::make_shared` 
function, but generally we can use any function that returns shared
pointer to our class.

## Registering class

After we defined `inspect` method and class name, we can register class
in duktape context;

```
ctx.registerClass<SpaceInvaders::Spaceship>();
```

## Evaluating script

```
ctx.evalStringNoRes("var spaceship = new SpaceInvaders.Spaceship(5)");
```

## Add global object

```
duk::Context ctx;
ctx.addGlobal<MyClass>();
```

## Custom data types

## Polymorphic classes

# What is supported (Roadmap)

- class binding with constructors, methods, constants, and properties (getters and setters)
- polymorphic types (with only one level of inheritance, e.g. interface + implementation)
- STL types (`std::vector`, `std::tuple`, `std::string`)
- shared pointers
- wrapping JS function into `std::function`
- custom data types

# License