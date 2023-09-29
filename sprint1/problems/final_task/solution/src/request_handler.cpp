#include "request_handler.h"
#include "json_keys.h"

namespace http_handler {
namespace detail {
using namespace json_keys;

std::string JsonBodyMaker::MakeBadRequestBody() {
    static std::string result = GetBadRequestBodyTxt();
    return result;
}

std::string JsonBodyMaker::MakeNotFoundBody() {
    static std::string result = GetNotFoundBodyTxt();
    return result;
}

std::string JsonBodyMaker::MakeOneMapBody(const model::Map& map) {
    return json::serialize(json::value_from(map));
}

std::string JsonBodyMaker::MakeMapsBody(const std::vector<model::MapInfo>& maps) {
    return json::serialize(json::value_from(maps));
}

std::string JsonBodyMaker::GetBadRequestBodyTxt () {
    json::value jv = {
        {code_key, "badRequest"},
        {message_key, "Bad request"}
    };
    return json::serialize(jv);
}

std::string JsonBodyMaker::GetNotFoundBodyTxt () {
    json::value jv = {
        {code_key, "mapNotFound"},
        {message_key, "Map not found"}
    };
    return json::serialize(jv);
}


std::unique_ptr<BodyMaker> GetBodyMaker(Format format) {
    if (format == Format::JSON) {
        return std::make_unique<JsonBodyMaker>();
    }
    throw std::logic_error("Type not available!");
}


/* Нашел небольшой баг: функция ищет строки, заключенные
 * между '/', т.е. строка "/api/v1/maps" вернет тоже самое,
 * что строка "///api/////v1////maps////". Нужно ли это поправить? */
std::vector<std::string_view> ParseTarget(std::string_view target) {
    std::vector<std::string_view> result;

    size_t start = target.find_first_not_of('/');
    size_t end = target.find_first_of('/', start);

    while (end != target.npos) {
        result.emplace_back(target.substr(start, end - start));
        start = target.find_first_not_of('/', end);
        end = target.find_first_of('/', start);
    }

    if (start != target.npos) {
        result.emplace_back(target.substr(start));
    }

    return result;
}
} // namespace detail
} // namespace http_handler

namespace model {
using namespace json_keys;

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
        {offices_key, map.GetOffices()}
    };
}

void tag_invoke (json::value_from_tag, json::value& jv, const MapInfo& map) {
    jv = {
        {id_key, map.GetId()},
        {name_key, map.GetName()}
    };
}
} // namespace model
