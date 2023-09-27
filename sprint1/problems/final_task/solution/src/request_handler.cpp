#include "request_handler.h"

namespace http_handler {

}  // namespace http_handler

namespace model {
void tag_invoke (json::value_from_tag, json::value& jv, const Road& road) {
    std::string end_coord_id;
    int end_coor_val;

    if (road.IsHorizontal()) {
        end_coord_id = "x1";
        end_coor_val = road.GetEnd().x;
    } else {
        end_coord_id = "y1";
        end_coor_val = road.GetEnd().y;
    }

    jv = {
        {"x0", road.GetStart().x},
        {"y0", road.GetStart().y},
        {end_coord_id, end_coor_val}
    };
}

void tag_invoke (json::value_from_tag, json::value& jv, const Building& building) {
    jv = {
        {"x", building.GetBounds().position.x},
        {"y", building.GetBounds().position.y},
        {"w", building.GetBounds().size.width},
        {"h", building.GetBounds().size.height}
    };
}

void tag_invoke (json::value_from_tag, json::value& jv, const Office& office) {
    jv = {
        {"id", *office.GetId()},
        {"x", office.GetPosition().x},
        {"y", office.GetPosition().y},
        {"offsetX", office.GetOffset().dx},
        {"offsetY", office.GetOffset().dy}
    };
}

void tag_invoke (json::value_from_tag, json::value& jv, const Map& map) {
    jv = {
        {"id", *map.GetId()},
        {"name", map.GetName()},
        {"roads", map.GetRoads()},
        {"buildings", map.GetBuildings()},
        {"offices", map.GetOffices()}
    };
}

void tag_invoke (json::value_from_tag, json::value& jv, const MapInfo& map) {
    jv = {
        {"id", map.GetId()},
        {"name", map.GetName()}
    };
}
}
