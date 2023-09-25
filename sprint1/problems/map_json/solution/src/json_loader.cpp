#include "json_loader.h"

#include <fstream>
#include <string>
#include <iostream>

#include <boost/json.hpp>

namespace json = boost::json;

namespace  {
std::string StringFromJson(const json::string& str) {
    return std::string(str.data(), str.size());
}

template <typename T>
typename T::Id GetId(const json::value& id) {
    return typename T::Id{StringFromJson(id.at("id").as_string())};
}

int GetInt(const json::value& value, json::string_view id) {
    return value.at(id).as_int64();
}

template <typename T>
std::vector<T> GetVector(const json::value& val, json::string_view id) {
    return value_to<std::vector<T>>(val.at(id));
}
} // namespace

namespace model {
Road tag_invoke(json::value_to_tag<Road>&, const json::value& road) {
    int x0 = GetInt(road, "x0");
    int y0 = GetInt(road, "y0");

    if (road.as_object().contains("x1")) {
        return Road {
            Road::HORIZONTAL,
            Point{x0, y0},
            GetInt(road, "x1")
        };
    } else {
        return model::Road {
            Road::VERTICAL,
            Point{x0, y0},
            GetInt(road, "y1")
        };
    }
}

Building tag_invoke(json::value_to_tag<Building>&, const json::value building) {
    return Building {
        Rectangle {
            Point {
                GetInt(building, "x"),
                GetInt(building, "y")
            },
            Size {
                GetInt(building, "w"),
                GetInt(building, "h")
            }
        }
    };
}

Office tag_invoke(json::value_to_tag<Office>&, const json::value office) {
    return Office {
        GetId<Office>(office),
        Point {
            GetInt(office, "x"),
            GetInt(office, "y")
        },
        Offset {
            GetInt(office, "offsetX"),
            GetInt(office, "offsetY")
        }
    };
}

Map tag_invoke(json::value_to_tag<Map>&, const json::value map) {
    Map result {
        GetId<Map>(map),
        StringFromJson(map.at("name").as_string())
    };

    auto roads = GetVector<Road>(map, "roads");
    auto buildings = GetVector<Building>(map, "buildings");
    auto offices = GetVector<Office>(map, "offices");

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

    auto maps = GetVector<model::Map>(config_json, "maps");

    for (auto& map : maps) {
        game.AddMap(map);
    }

    return game;
}
}  // namespace json_loader







