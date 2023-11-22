#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <vector>


#include "../src/move_manager.h"
#include "../src/game_manager.h"
#include "../src/game_serialization.h"

namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}  // namespace

void CheckEqPos (const move_manager::PositionState& lhs, const move_manager::PositionState& rhs) {
    CHECK(lhs.coor == rhs.coor);
    CHECK((lhs.area == nullptr || rhs.area == nullptr));
}

void CheckEqLootObj(const game_manager::LootObject& lhs, const game_manager::LootObject& rhs) {
    CHECK(lhs.collected == rhs.collected);
    CHECK(lhs.id == rhs.id);
    CHECK(lhs.type == rhs.type);
    CHECK(lhs.position == rhs.position);
}

void CheckEqPlayers(const game_manager::Player& lhs, const game_manager::Player& rhs) {
    CHECK(lhs.id == rhs.id);
    CHECK(lhs.items_in_bag == rhs.items_in_bag);
    CHECK(lhs.name == rhs.name);
    CHECK(lhs.score == rhs.score);
    CHECK(lhs.state.dir == rhs.state.dir);
    CHECK(lhs.state.speed == rhs.state.speed);
    CheckEqPos(lhs.state.position, rhs.state.position);
}

SCENARIO_METHOD(Fixture, "Coords serialization") {
    GIVEN("A coords") {
        const move_manager::Coords coords{10.6 , 20.25};
        WHEN("coords is serialized") {
            output_archive << coords;

            THEN("it is equal to coords after serialization") {
                InputArchive input_archive{strm};
                move_manager::Coords restored_coords;
                input_archive >> restored_coords;
                CHECK(coords == restored_coords);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Speed serialization") {
    GIVEN("A speed") {
        const move_manager::Speed speed{8.7, 0.567};
        WHEN("speed is serialized") {
            output_archive << speed;

            THEN("it is equal to speed after serialization") {
                InputArchive input_archive{strm};
                move_manager::Speed restored_speed;
                input_archive >> restored_speed;
                CHECK(speed == restored_speed);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Position serialization") {
    GIVEN("A position") {
        move_manager::Area area({1, 1});
        const move_manager::PositionState pos{{1.6, 2.6}, &area};
        WHEN("position is serialized") {
            output_archive << pos;

            THEN("it coords is equal to coords after serialization and area == nullptr") {
                InputArchive input_archive{strm};
                move_manager::PositionState restored_pos;
                input_archive >> restored_pos;
                CheckEqPos(pos, restored_pos);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "LootObject serialization") {
    GIVEN("A LootObject") {
        const move_manager::Area area({1, 1});
        const move_manager::Coords pos{1.8, 2.3};
        const game_manager::LootObject item{3, pos, 4, false};
        WHEN("LootObject is serialized") {
            output_archive << item;

            THEN("it is equal to LootObject after serialization") {
                InputArchive input_archive{strm};
                game_manager::LootObject restored_item;
                input_archive >> restored_item;
                CheckEqLootObj(item, restored_item);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "LootObjects vector serialization") {
    GIVEN("A LootObjects vector") {

        const move_manager::Coords pos1{1.23, 2.4567};
        const move_manager::Coords pos2{3.67, 4.34};
        const move_manager::Coords pos3{5.78, 6.876};

        std::vector<game_manager::LootObject> items;
        items.emplace_back(1, pos1, 0, false);
        items.emplace_back(1, pos2, 1, true);
        items.emplace_back(2, pos3, 2, false);

        WHEN("LootObjects vector is serialized") {
            output_archive << items;

            THEN("it is equal to LootObjects after serialization") {
                InputArchive input_archive{strm};
                std::vector<game_manager::LootObject> restored_items;
                input_archive >> restored_items;

                REQUIRE(items.size() == restored_items.size());

                for (size_t i = 0; i < items.size(); ++i) {
                    CheckEqLootObj(items.at(i), restored_items.at(i));
                }
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "ItemInfo serialization") {
    GIVEN("A ItemInfo") {
        const game_manager::ItemInfo info{3, 4};
        WHEN("ItemInfo is serialized") {
            output_archive << info;

            THEN("it is equal to ItemInfo after serialization") {
                InputArchive input_archive{strm};
                game_manager::ItemInfo restored_info;
                input_archive >> restored_info;
                CHECK(info == restored_info);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Player serialization") {
    GIVEN("A Player") {
        game_manager::Player player;

        const move_manager::Area area({1, 2});
        const move_manager::PositionState pos({{5, 8}, &area});

        move_manager::State state;
        state.dir = move_manager::Direction::WEST;
        state.position = pos;
        state.speed = {-5.6, 0};

        player.id = 4;
        player.name = "Player";
        player.score = 44;
        player.items_in_bag.emplace_back(1, 4);
        player.items_in_bag.emplace_back(7, 2);
        player.state = state;

        WHEN("Player is serialized") {
            output_archive << player;

            THEN("it is equal to Player after serialization") {
                InputArchive input_archive{strm};
                game_manager::Player restored_player;
                input_archive >> restored_player;
                CheckEqPlayers(player, restored_player);
            }
        }
    }
}
