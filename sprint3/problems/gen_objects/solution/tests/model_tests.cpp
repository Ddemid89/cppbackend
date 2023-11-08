#include "../src/game_manager.h"

#include <boost/asio/io_context.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>

#include <thread>

#include <optional>

#include <iostream>

namespace net = boost::asio;

model::LootType MakeLootType(std::string name) {
    model::LootType res;
    res.file = "some_dir/" + name + ".obj";
    res.name = std::move(name);
    res.type = model::LootSort::obj;
    res.scale = 0.5;

    return res;
}

game_manager::PlayerInfo MakePlayer() {
    static int id = 0;
    game_manager::PlayerInfo res;
    res.Id = id++;
    res.name = "Player_" + std::to_string(id);
    return res;
}

struct TestData {
    model::Map map{model::Map::Id{"0"}, "my_map", 10};
    std::vector<model::LootType> loot_types;
    model::LootConfig loot_config;
    std::optional<move_manager::Map> move_map;
};

TestData GetData(size_t types, const std::vector<model::Road>& roads, double period = 5.0, double prob = 0.5) {
    TestData res;

    res.loot_types.reserve(types);

    for (const model::Road& road : roads) {
        res.map.AddRoad(road);
    }

    res.loot_config.period = period;
    res.loot_config.probability = prob;

    for (int i = 0; i < types; ++i) {
        res.loot_types.push_back(MakeLootType("type" + std::to_string(i)));
        res.map.AddLootType(res.loot_types.back());
    }

    res.move_map = move_manager::Map{res.map};

    return res;
}

void AddPlayers(game_manager::GameSession& session, size_t number) {
    for (size_t i = 0; i < number; i++) {
        session.AddPlayer(MakePlayer(), [](const game_manager::PlayerInfo&){});
    }
}

void MakeTicks(game_manager::GameSession& session, uint64_t dur, size_t number) {
    static int id = 0;
    for (int i = 0; i < number; i++) {
        session.Tick(dur);
    }
}

void CheckCount(game_manager::GameSession& session, size_t expected_count) {
    session.GetPlayers([expected_count](game_manager::PlayersAndObjects& res, game_manager::Result){
        CHECK(res.objects.size() == expected_count);
    });
}

void CheckTypes(game_manager::GameSession& session, size_t loot_count) {
    session.GetPlayers([loot_count](game_manager::PlayersAndObjects& res, game_manager::Result){
        for (const game_manager::LootObject& obj : res.objects) {
            CHECK(obj.type < loot_count);
        }
    });
}

bool CheckOneObjectOneRoad(const game_manager::LootObject& obj, const model::Road& road) {
    int x1 = road.GetStart().x;
    int x2 = road.GetEnd().x;
    int y1 = road.GetStart().y;
    int y2 = road.GetEnd().y;
    double l = std::min(x1, x2) - 0.4;
    double r = std::max(x1, x2) + 0.4;
    double t = std::min(y1, y2) - 0.4;
    double b = std::max(y1, y2) + 0.4;

    return l <= obj.position.coor.x && obj.position.coor.x <= r
            && t <= obj.position.coor.y && obj.position.coor.y <= b;

}

bool CheckOneObjectPosition(const game_manager::LootObject& obj,  std::vector<model::Road>& roads) {
    for (const model::Road& road : roads) {
        if (CheckOneObjectOneRoad(obj, road)) {
            return true;
        }
    }
    return false;
}

void CheckPosition(game_manager::GameSession& session, std::vector<model::Road>& roads) {
    session.GetPlayers([&roads](game_manager::PlayersAndObjects& res, game_manager::Result){
        for (const game_manager::LootObject& obj : res.objects) {
            CHECK(CheckOneObjectPosition(obj, roads));
        }
    });
}

void CheckSession(game_manager::GameSession& session, size_t players_to_add, size_t loot_count, std::vector<model::Road>& roads) {
    using namespace std::literals;

    static size_t players_count = 0;

    AddPlayers(session, players_to_add);
    players_count += players_to_add;
    MakeTicks(session, 1000, players_to_add * 7);
    CheckCount(session, players_count);
    CheckTypes(session, loot_count);
    CheckPosition(session, roads);
}

TEST_CASE("model_tests") {
    using namespace std::literals;

    net::io_context ioc;

    std::atomic<bool> done = false;

    const int LOOT_COUNT = 5;

    std::vector<model::Road> roads;

    roads.emplace_back(model::Road::HORIZONTAL, model::Point{0, 0}, 5);
    roads.emplace_back(model::Road::VERTICAL, model::Point{0, 0}, 5);

    auto data = GetData(LOOT_COUNT, roads);

    game_manager::GameSession session{ioc, data.map, *data.move_map, data.loot_config, true};

    std::thread worker([&ioc, &done] {
        while (!done) {
            ioc.run();
        }
    });

    CheckCount(session, 0);

    CheckSession(session,  2, LOOT_COUNT, roads);
    CheckSession(session,  5, LOOT_COUNT, roads);
    CheckSession(session,  3, LOOT_COUNT, roads);
    CheckSession(session, 90, LOOT_COUNT, roads);

    done = true;
    worker.join();

}
