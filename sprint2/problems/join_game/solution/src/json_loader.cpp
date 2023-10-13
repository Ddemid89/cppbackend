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
    std::ifstream out(json_path);
    std::string json_str;

    if (out.is_open()) {
        json_str = std::string(std::istreambuf_iterator<char>(out), std::istreambuf_iterator<char>());
    } else {
        throw std::runtime_error("The configuration file could not be opened!");
    }

    model::Game game;

    auto config_json = json::parse(json_str);

    auto maps = json::value_to<std::vector<model::Map>>(config_json.at(json_keys::maps_key));

    for (auto& map : maps) {
        game.AddMap(map);
    }

    return game;
}
}  // namespace json_loader
