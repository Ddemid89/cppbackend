#include "json_loader.h"

#include <fstream>
#include <string>

#include <boost/json.hpp>

namespace json = boost::json;

namespace json_loader {

namespace  {

std::string StringFromJson(json::string str) {
    return std::string(str.data(), str.size());
}

model::Road RoadFromJson(const json::value& road) {
    model::Coord x0 = road.at("x0").as_int64();
    model::Coord y0 = road.at("y0").as_int64();

    if (road.as_object().contains("x1")) {
        model::Coord x1 = road.at("x1").as_int64();
        return {model::Road::HORIZONTAL, {x0, y0}, x1};
    } else {
        model::Coord y1 = road.at("y1").as_int64();
        return {model::Road::VERTICAL, {x0, y0}, y1};
    }
}

model::Building BuildingFromJson(const json::value& building) {
    model::Coord x = building.at("x").as_int64();
    model::Coord y = building.at("y").as_int64();

    model::Dimension w = building.at("w").as_int64();
    model::Dimension h = building.at("h").as_int64();

    return model::Building{{{x, y}, {w, h}}};
}

model::Office OfficeFromJson(const json::value& office) {
    std::string id_str = StringFromJson(office.at("id").as_string());
    model::Office::Id id(id_str);

    model::Coord x = office.at("x").as_int64();
    model::Coord y = office.at("y").as_int64();

    model::Dimension offsetX = office.at("offsetX").as_int64();
    model::Dimension offsetY = office.at("offsetY").as_int64();

    model::Point coors{x, y};
    model::Offset offset{offsetX, offsetY};

    return {id, coors, offset};
}

model::Map MapFromJson(const json::value& map) {
    std::string id_str = StringFromJson(map.at("id").as_string());
    model::Map::Id id(id_str);

    std::string name = StringFromJson(map.at("name").as_string());

    std::string std_name(name.begin(), name.end());

    model::Map result(id, std_name);

    auto& roads     = map.at("roads").as_array();
    auto& buildings = map.at("buildings").as_array();
    auto& offices   = map.at("offices").as_array();

    for (auto& road : roads) {
        result.AddRoad(RoadFromJson(road));
    }

    for (auto& building : buildings) {
        result.AddBuilding(BuildingFromJson(building));
    }

    for (auto& office : offices) {
        result.AddOffice(OfficeFromJson(office));
    }

    return result;
}
} // namespace

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

    auto& maps_array_json = config_json.at("maps").as_array();

    for (auto& map : maps_array_json) {
        game.AddMap(MapFromJson(map));
    }

    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла

    return game;
}

}  // namespace json_loader
