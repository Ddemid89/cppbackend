#include "json_loader.h"

#include <fstream>
#include <string>
#include <iostream>

#include <boost/json.hpp>

#include "json_keys.h"

namespace json = boost::json;

namespace  {
using namespace json_keys;

std::string StringFromJson(const json::string& str) {
    return std::string(str.data(), str.size());
}

template <typename T>
typename T::Id GetId(const json::value& id) {
    return typename T::Id{StringFromJson(id.at(id_key).as_string())};
}

int GetInt(const json::value& value, json::string_view id) {
    return value.at(id).as_int64();
}
} // namespace

namespace model {
Road tag_invoke(json::value_to_tag<Road>&, const json::value& road) {
    int x0 = GetInt(road, x0_key);
    int y0 = GetInt(road, y0_key);

    if (road.as_object().contains(x1_key)) {
        return Road {
            Road::HORIZONTAL,
            Point{x0, y0},
            GetInt(road, x1_key)
        };
    } else {
        return model::Road {
            Road::VERTICAL,
            Point{x0, y0},
            GetInt(road, y1_key)
        };
    }
}

Building tag_invoke(json::value_to_tag<Building>&, const json::value building) {
    return Building {
        Rectangle {
            Point {
                GetInt(building, x_key),
                GetInt(building, y_key)
            },
            Size {
                GetInt(building, width_key),
                GetInt(building, height_key)
            }
        }
    };
}

Office tag_invoke(json::value_to_tag<Office>&, const json::value office) {
    return Office {
        GetId<Office>(office),
        Point {
            GetInt(office, x_key),
            GetInt(office, y_key)
        },
        Offset {
            GetInt(office, offsetX_key),
            GetInt(office, offsetY_key)
        }
    };
}

Map tag_invoke(json::value_to_tag<Map>&, const json::value map) {
    Map result {
        GetId<Map>(map),
        StringFromJson(map.at(name_key).as_string())
    };

    auto roads = json::value_to<std::vector<Road>>(map.at(roads_key));
    auto buildings = json::value_to<std::vector<Building>>(map.at(buildings_key));
    auto offices = json::value_to<std::vector<Office>>(map.at(offices_key));

    for (auto& road : roads) {
        result.AddRoad(road);
    }

    for (auto& building : buildings) {
        result.AddBuilding(building);
    }

    for (auto& office : offices) {
        result.AddOffice(std::move(office));
    }

    return result;
}
} // namespace model


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

    auto maps = json::value_to<std::vector<model::Map>>(config_json.at(maps_key));

    for (auto& map : maps) {
        game.AddMap(map);
    }

    return game;
}
}  // namespace json_loader
