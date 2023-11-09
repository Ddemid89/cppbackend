#pragma once

#include <boost/json.hpp>
#include "model.h"

namespace model {
namespace json = boost::json;

void tag_invoke (json::value_from_tag, json::value& jv, const Road& road);

void tag_invoke (json::value_from_tag, json::value& jv, const Building& building);

void tag_invoke (json::value_from_tag, json::value& jv, const Office& office);

void tag_invoke (json::value_from_tag, json::value& jv, const Map& map);

void tag_invoke (json::value_from_tag, json::value& jv, const MapInfo& map);

Road tag_invoke(json::value_to_tag<Road>&, const json::value& road);

Building tag_invoke(json::value_to_tag<Building>&, const json::value building);

Office tag_invoke(json::value_to_tag<Office>&, const json::value office);

Map tag_invoke(json::value_to_tag<Map>&, const json::value map);

LootConfig tag_invoke(json::value_to_tag<LootConfig>&, const json::value config);

LootType tag_invoke(json::value_to_tag<LootType>&, const json::value type);

void tag_invoke (json::value_from_tag, json::value& jv, const LootSort& type);

void tag_invoke (json::value_from_tag, json::value& jv, const LootType& type);

void tag_invoke (json::value_from_tag, json::value& jv, const Road& road);//??????????????????????????????????????????

void tag_invoke (json::value_from_tag, json::value& jv, const Building& building);

void tag_invoke (json::value_from_tag, json::value& jv, const Office& office);

void tag_invoke (json::value_from_tag, json::value& jv, const Map& map);

void tag_invoke (json::value_from_tag, json::value& jv, const MapInfo& map);
} // namespace model
