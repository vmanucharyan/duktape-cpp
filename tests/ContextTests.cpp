#include <catch/catch.hpp>

#include <duktape-cpp/DuktapeCpp.h>

namespace ContextTests {

class Player {
public:
    static std::shared_ptr<Player> CreatePlayer(int id) {
        return std::make_shared<Player>(id);
    }

    Player() {}

    explicit Player(int id): _id(id) {}

    template <class Inspector>
    static void inspect(Inspector &i) {
        i.construct(&Player::CreatePlayer);
        i.property("id", &Player::id, &Player::setId);
    }

    int id() const { return this->_id; }
    void setId(int id) { this->_id = id; }

    int _id {0};
};

enum class TestEnum {
    VALUE_1 = 1,
    VALUE_2 = 2
};

class Players {
public:
    Players() = default;

    void addPlayer(const std::shared_ptr<Player> &player) {
        _players.push_back(player);
    }

    std::shared_ptr<Player> getPlayer(int id) {
        std::shared_ptr<Player> res;

        auto v = std::find_if(
            _players.begin(), _players.end(),
            [this, id] (auto p) { return p->id() == id; }
        );

        if (v != _players.end()) {
            res = *v;
        }

        return res;
    }

    template <class Inspector>
    static void inspect(Inspector &i) {
        i.method("addPlayer", &Players::addPlayer);
        i.method("getPlayer", &Players::getPlayer);
    }

private:
    std::vector<std::shared_ptr<Player>> _players;
};

}

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

DUK_CPP_DEF_CLASS_NAME(ContextTests::Player);
DUK_CPP_DEF_CLASS_NAME(ContextTests::TestEnum);

TEST_CASE("Context", "[duktape]") {
    using namespace ContextTests;

    duk::Context ctx;

    SECTION("registerClass") {
        SECTION("should register class constructor") {
            const char script[] = "new ContextTests.Player(9)";

            ctx.registerClass<ContextTests::Player>();

            duk_eval_string(ctx, script);

            std::shared_ptr<ContextTests::Player> popped;
            duk::Type<std::shared_ptr<ContextTests::Player>>::get(ctx,  popped, -1);
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

        auto players = std::make_shared<Players>();

        d.registerClass<Player>();
        d.addGlobal("Players", players);

        SECTION("should push shared ptr objects") {
            const char script[] =
                "var p = new ContextTests.Player(5);\n"
                "Players.addPlayer(p);\n"
                "Players.getPlayer(5)";

            duk_eval_string(d, script);

            std::shared_ptr<Player> stackPlayer;
            duk::Type<std::shared_ptr<Player>>::get(d, stackPlayer, -1);

            REQUIRE(stackPlayer->id() == 5);
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
