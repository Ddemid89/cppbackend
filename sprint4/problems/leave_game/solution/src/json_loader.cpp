#include "json_loader.h"
#include "model_serialization.h"

#include <fstream>
#include <string>
#include <iostream>

#include <boost/json.hpp>

#include "json_keys.h"

namespace json = boost::json;

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::ifstream input(json_path);
    std::string json_str;

    if (input.is_open()) {
        json_str = std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    } else {
        throw std::runtime_error("The configuration file could not be opened!");
    }

    auto config_json = json::parse(json_str);

    model::GameConfig game_config;

    game_config.default_speed = model::DEFAULT_DOG_SPEED;

    if (config_json.as_object().contains(json_keys::game_def_speed_key)) {
        game_config.default_speed = config_json.as_object().at(json_keys::game_def_speed_key).as_double();
    }

    game_config.default_capacity = model::DEFAULT_BAG_CAPACITY;

    if (config_json.as_object().contains(json_keys::default_bag_capacity_key)) {
        game_config.default_capacity = config_json.as_object().at(json_keys::default_bag_capacity_key).as_uint64();
    }

    game_config.retirement_time = model::DEFAULT_RETIREMENT_TIME;

    if (config_json.as_object().contains(json_keys::retirement_time_key)) {
        game_config.retirement_time = config_json.as_object().at(json_keys::retirement_time_key).as_double();
    }

    game_config.loot_config = json::value_to<model::LootConfig>(config_json.at(json_keys::loot_config_key));

    model::Game game(game_config);

    auto maps = json::value_to<std::vector<model::Map>>(config_json.at(json_keys::maps_key));

    for (auto& map : maps) {
        game.AddMap(map);
    }

    return game;
}
}  // namespace json_loader
