#include "request_handler.h"
#include "json_keys.h"

namespace http_handler {
namespace body_type {
std::string to_lower_case(std::string_view str) {
    static int diff = 'A' - 'a';
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        if ('A' <= c && c <= 'Z') {
            result.push_back(c - diff);
        } else {
            result.push_back(c);
        }
    }
    return result;
}

const std::string& GetTypeByExtention(std::string_view file) {
    static std::unordered_map<std::string, const std::string*> types = {{"htm"s, &html},
                                                                 {"html"s, &html},
                                                                 {"css"s,  &css},
                                                                 {"txt"s,  &txt},
                                                                 {"js"s,   &js},
                                                                 {"json"s, &json},
                                                                 {"xml"s,  &xml},
                                                                 {"png"s,  &png},
                                                                 {"jpg"s,  &jpg},
                                                                 {"jpe"s,  &jpg},
                                                                 {"jpeg"s, &jpg},
                                                                 {"gif"s,  &gif},
                                                                 {"bmp"s,  &bmp},
                                                                 {"ico"s,  &ico},
                                                                 {"tiff"s, &tif},
                                                                 {"tif"s,  &tif},
                                                                 {"svg"s,  &svg},
                                                                 {"svgz"s, &svg},
                                                                 {"mp3"s,  &mp3}};
    size_t last_dot = file.find_last_of('.');
    if (last_dot == file.npos || last_dot == file.size() - 1) {
        return unknown;
    }
    std::string extention = to_lower_case(file.substr(last_dot + 1));

    auto it = types.find(extention);

    if (it == types.end()) {
        return unknown;
    } else {
        return *it->second;
    }
}
} // namespace body_type

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
