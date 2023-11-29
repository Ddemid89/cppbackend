#pragma once

#include <string>

namespace json_keys {
using namespace std::literals;

const std::string maps_key      = "maps"s;
const std::string roads_key     = "roads"s;
const std::string buildings_key = "buildings"s;
const std::string offices_key   = "offices"s;

const std::string map_dog_speed_key    = "dogSpeed"s;
const std::string game_def_speed_key   = "defaultDogSpeed"s;
const std::string default_bag_capacity_key = "defaultBagCapacity"s;
const std::string map_bag_capacity_key = "bagCapacity"s;

const std::string id_key   = "id"s;
const std::string name_key = "name"s;

const std::string x_key  = "x"s;
const std::string y_key  = "y"s;
const std::string x0_key = "x0"s;
const std::string y0_key = "y0"s;
const std::string x1_key = "x1"s;
const std::string y1_key = "y1"s;

const std::string width_key  = "w"s;
const std::string height_key = "h"s;

const std::string offsetX_key = "offsetX"s;
const std::string offsetY_key = "offsetY"s;

const std::string code_key    = "code"s;
const std::string message_key = "message"s;

const std::string bad_request_key     = "badRequest"s;
const std::string bad_request_message = "Bad request"s;

const std::string not_found_key     = "mapNotFound"s;
const std::string not_found_message = "Map not found"s;

const std::string auth_token_key = "authToken"s;
const std::string player_id_key  = "playerId"s;

const std::string invalid_method_key     = "invalidMethod"s;
const std::string invalid_method_message_post = "Only POST method is expected"s;
const std::string invalid_method_message_get_head = "Only GET/HEAD method is expected"s;

const std::string bad_token_key      = "invalidToken"s;
const std::string bad_token_mess     = "Something wrong with token"s;

const std::string invalid_token_key  = "invalidToken"s;
const std::string unknown_token_key  = "unknownToken"s;
const std::string invalid_token_mess = "Authorization header is missing"s;
const std::string unknown_token_mess = "Player token has not been found"s;

const std::string token_prefix = "Bearer "s;

const std::string pos_key     = "pos"s;
const std::string speed_key   = "speed"s;
const std::string dir_key     = "dir"s;
const std::string players_key = "players"s;

const std::string user_name_key  = "userName"s;
const std::string map_id_key     = "mapId"s;
const std::string time_delta_key = "timeDelta"s;
const std::string move_key = "move"s;

const std::string loot_config_key = "lootGeneratorConfig"s;
const std::string loot_period_key = "period"s;
const std::string loot_probablity_key = "probability"s;

const std::string loot_types_key = "lootTypes"s;
const std::string file_key = "file"s;
const std::string type_key = "type"s;
const std::string rotation_key = "rotation"s;
const std::string color_key = "color"s;
const std::string scale_key = "scale"s;

const std::string lost_objects_key = "lostObjects"s;
const std::string bag_key = "bag"s;
const std::string value_key = "value"s;
const std::string score_key = "score"s;

const std::string retirement_time_key = "dogRetirementTime"s;

const std::string max_items_key = "maxItems"s;
const std::string start_key     = "start"s;

const std::string playtime_key  = "playTime"s;


} // namespace json_keys
