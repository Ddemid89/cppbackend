#include "model_serialization.h"

#include <boost/json.hpp>
#include "model.h"
#include "json_keys.h"

namespace model {
namespace json = boost::json;
using namespace json_keys;

namespace detail {

std::string StringFromJson(const json::string& str) {
    return std::string(str.data(), str.size());
}

int GetInt(const json::value& value, json::string_view id) {
    return value.at(id).as_int64();
}

template <typename T>
typename T::Id GetId(const json::value& id) {
    return typename T::Id{StringFromJson(id.at(id_key).as_string())};
}

LootSort GetLootSort(std::string_view str) {
    if (str == "obj"sv) {
        return LootSort::obj;
    }
    throw std::logic_error("Other loot types not implemented");
}

} // namespace detail


LootConfig tag_invoke(json::value_to_tag<LootConfig>&, const json::value config) {
    double period = config.at(json_keys::loot_period_key).as_double();
    double prob = config.at(json_keys::loot_probablity_key).as_double();
    LootConfig res;
    res.period = period;
    res.probability = prob;
    return res;
}

Road tag_invoke(json::value_to_tag<Road>&, const json::value& road) {
    int x0 = detail::GetInt(road, x0_key);
    int y0 = detail::GetInt(road, y0_key);

    if (road.as_object().contains(x1_key)) {
        return Road {
            Road::HORIZONTAL,
            Point{x0, y0},
            detail::GetInt(road, x1_key)
        };
    }
    return model::Road {
        Road::VERTICAL,
        Point{x0, y0},
        detail::GetInt(road, y1_key)
    };

}

Building tag_invoke(json::value_to_tag<Building>&, const json::value building) {
    return Building {
        Rectangle {
            Point {
                detail::GetInt(building, x_key),
                detail::GetInt(building, y_key)
            },
            Size {
                detail::GetInt(building, width_key),
                detail::GetInt(building, height_key)
            }
        }
    };
}

Office tag_invoke(json::value_to_tag<Office>&, const json::value office) {
    return Office {
        detail::GetId<Office>(office),
        Point {
            detail::GetInt(office, x_key),
            detail::GetInt(office, y_key)
        },
        Offset {
            detail::GetInt(office, offsetX_key),
            detail::GetInt(office, offsetY_key)
        }
    };
}

LootType tag_invoke(json::value_to_tag<LootType>&, const json::value type) {
    LootType res;
    res.name = type.at(json_keys::name_key).as_string();
    res.file = type.at(json_keys::file_key).as_string();
    res.scale = type.at(json_keys::scale_key).as_double();

    if (type.as_object().contains(json_keys::rotation_key)) {
            res.rotation = type.at(json_keys::rotation_key).as_int64();
    }

    if (type.as_object().contains(json_keys::color_key)) {
            res.color = type.at(json_keys::color_key).as_string();
    }

    res.type = detail::GetLootSort(type.at(json_keys::type_key).as_string());
    return res;
}

Map tag_invoke(json::value_to_tag<Map>&, const json::value map) {
    double speed = 0;
    if (map.as_object().contains(map_dog_speed_key)) {
        speed = map.as_object().at(map_dog_speed_key).as_double();
    }

    Map result {
        detail::GetId<Map>(map),
        detail::StringFromJson(map.at(name_key).as_string()),
        speed
    };

    auto roads = json::value_to<std::vector<Road>>(map.at(roads_key));
    auto buildings = json::value_to<std::vector<Building>>(map.at(buildings_key));
    auto offices = json::value_to<std::vector<Office>>(map.at(offices_key));
    auto loot_types = json::value_to<std::vector<LootType>>(map.at(json_keys::loot_types_key));

    for (auto& road : roads) {
        result.AddRoad(road);
    }

    for (auto& building : buildings) {
        result.AddBuilding(building);
    }

    for (auto& office : offices) {
        result.AddOffice(std::move(office));
    }

    for (auto& loot_type : loot_types) {
        result.AddLootType(loot_type);
    }

    return result;
}

void tag_invoke (json::value_from_tag, json::value& jv, const Road& road) {
    std::string end_coord_id;
    int end_coor_val;

    if (road.IsHorizontal()) {
        end_coord_id = x1_key;
        end_coor_val = road.GetEnd().x;
    } else {
        end_coord_id = y1_key;
        end_coor_val = road.GetEnd().y;
    }

    jv = {
        {x0_key, road.GetStart().x},
        {y0_key, road.GetStart().y},
        {end_coord_id, end_coor_val}
    };
}

void tag_invoke (json::value_from_tag, json::value& jv, const Building& building) {
    jv = {
        {x_key, building.GetBounds().position.x},
        {y_key, building.GetBounds().position.y},
        {width_key, building.GetBounds().size.width},
        {height_key, building.GetBounds().size.height}
    };
}

void tag_invoke (json::value_from_tag, json::value& jv, const Office& office) {
    jv = {
        {id_key, *office.GetId()},
        {x_key, office.GetPosition().x},
        {y_key, office.GetPosition().y},
        {offsetX_key, office.GetOffset().dx},
        {offsetY_key, office.GetOffset().dy}
    };
}

void tag_invoke (json::value_from_tag, json::value& jv, const Map& map) { 
    jv = {
        {id_key, *map.GetId()},
        {name_key, map.GetName()},
        {roads_key, map.GetRoads()},
        {buildings_key, map.GetBuildings()},
        {offices_key, map.GetOffices()},
        {loot_types_key, map.GetLootTypes()}
    };
}

void tag_invoke (json::value_from_tag, json::value& jv, const MapInfo& map) {
    jv = {
        {id_key, map.GetId()},
        {name_key, map.GetName()}
    };
}

void tag_invoke(json::value_from_tag, json::value& jv, const LootType& type) {
    jv = {
        {name_key,     type.name},
        {file_key,     type.file},
        {type_key,     type.type},
        {scale_key,    type.scale}
    };

    if (type.color != "") {
        jv.as_object()[color_key] = type.color;
    }
    if (type.rotation.has_value()) {
        jv.as_object()[rotation_key] = *type.rotation;
    }

}

void tag_invoke(json::value_from_tag, json::value& jv, const LootSort& type) {
    if (type == LootSort::obj) {
        jv = json::string{"obj"};
    } else {
        throw std::invalid_argument("Not implemented");
    }

}

} // namespace model
