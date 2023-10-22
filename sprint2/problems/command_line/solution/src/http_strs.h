#pragma once

#include <string>

namespace http_strs {
using namespace std::literals;

const std::string api = "/api"s;
const std::string index_htm = "index.htm"s;
const std::string index_html = "index.html"s;
const std::string game_path = "/api/v1/game"s;
const std::string join = "/join"s;
const std::string players = "/players"s;
const std::string state = "/state"s;
const std::string player_action = "/player/action"s;
const std::string tick = "/tick"s;
} // namespace http_strs
