[![Build Status](https://travis-ci.org/vmanucharyan/duktape-cpp.svg?branch=master)](https://travis-ci.org/vmanucharyan/duktape-cpp)

# Summary

`duktape-cpp` is a library to easily bind C++ classes to [duktape](http://duktape.org) 
scripts. It aims to be easy to use and safe.

It is currently work-in-progress and may lack some features, so feel free to to contribute!

# Getting started

Requirements:
- Modern C++ compiler with C++14 support

duktape-cpp is a header-only library, so all you need is to just add source
files from `src` directory to you project and `#include "duktape-cpp/DuktapeCpp.h"`.

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
         * Get pointer to spaceship
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
This can be done by defining `inspect` method inside of our class.

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

```cpp
DUK_CPP_DEF_CLASS_NAME(SpaceInvaders::Spaceship)
```

Full name will be translated into `SpaceInvaders.Spaceship`.

We can specify another (shorter) name by using `DUK_CPP_DEF_SHORT_NAME` macro

```cpp
DUK_CPP_DEF_SHORT_NAME(SpaceInvaders::Spaceship, "Spaceship")
```

## Constructors

If we want to be able to create a spaceship from javascript code,
we have to define a constructor like this:

```cpp
template <class Inspector>
static void inspect(Inspector &i) {
    i.construct(&std::make_shared<SpaceInvaders::Spaceship, int>);
}
```

As a constructor, we specify a function that returns shared pointer
to our class. In this example, we simply use `std::make_shared` 
function, but generally we can use any function that returns shared
pointer to our class.

## Registering class

After we defined `inspect` method and class name, we can register class
in duktape context;

```cpp
ctx.registerClass<SpaceInvaders::Spaceship>();
```

## Evaluating script

There are two wrapper methods in `duk::Context` to evaluate script - 
`evaluateScript` and `evaluateScriptNoRes`. 

```cpp
std::shared_ptr<Spaceship> res;
ctx.evalString(res, "new SpaceInvaders.Spaceship(5)");
```

```cpp
ctx.evalStringNoRes("var spaceship = new SpaceInvaders.Spaceship(5)");
```

## Pass objects to script

To pass object to duktape context use `duk::Context::addGlobal` method.

```cpp
duk::Context ctx;

auto spaceship = std::make_shared<Spaceship>();
ctx.addGlobal("spaceship", spaceship);
```

Now we can access spaceship's methods and properties from script.

```cpp
ctx.evalStringNoRes("spaceship.moveRight()");
assert(spaceship->pos() == 6);
```

## Custom value types

Built-in value types:
- int, double, float, bool
- std::string
- std::shared_ptr
- std::unique_ptr
- std::vector
- std::tuple

You can define your own data type by specializing `duk::Type` template. 
Here is an example:

```cpp
struct Vec3 {
    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    float x {0.0f};
    float y {0.0f};
    float z {0.0f};
};

namespace duk {

template <>
struct Type<Vec3> {
    static void push(duk::Context &d, Vec3 const &val) {
        duk_push_object(d);

        duk_push_number(d, val.x);
        duk_put_prop_string(d, -2, "x");

        duk_push_number(d, val.y);
        duk_put_prop_string(d, -2, "y");

        duk_push_number(d, val.z);
        duk_put_prop_string(d, -2, "z");
    }

    static void get(duk::Context &d, Vec3 &val, int index) {
        duk_get_prop_string(d, index, "x");
        float x = float(duk_get_number(d, -1));
        duk_pop(d);

        duk_get_prop_string(d, index, "y");
        float y = float(duk_get_number(d, -1));
        duk_pop(d);

        duk_get_prop_string(d, index, "z");
        float z = float(duk_get_number(d, -1));
        duk_pop(d);

        val = Vec3(x, y, z);
    }

    static constexpr bool isPrimitive() { return true; }
};

}
```

As you can see, it is simply a converter to/from javascript native type.

`push` method defines how to push value to duktape's stack

`get` method defines how to get value from stack 
(`index` variable defines objects position in stack)

Please, refer to [duktape](http://duktape.org/index.html) documentation for
details about stack manipulation.

`static constexpr bool isPrimitive() { return true; }` indicates, that this
type is primitive. Primitive types are always passed to/from duktape context
by value.

## Polymorphic classes

`duktape-cpp` supports polymorphic types, but currently with only
one level of inheritance (e.g. interface and implementation).

Use `DUK_CPP_DEF_BASE_CLASS(type, base)` macro to define a base class.

After that, methods and properties of base class will be automatically
exposed in javascript (note, that base class also need to have `inspect` method
or specialize `Inspect` template).

See [tests/PolymorphicTypesTests.cpp](tests/PolymorphicTypesTests.cpp) for an example.

# How to build tests and examples

```
git clone https://github.com/vmanucharyan/duktape-cpp.git
cd duktape-cpp
mkdir build && cd build
cmake ..
make
```

# License (MIT)

Copyright (c) 2017 Vardan Manucharyan <sd003gm@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.
