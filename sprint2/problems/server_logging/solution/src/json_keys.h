#pragma once

#include <string>

namespace json_keys {
using namespace std::literals;

const std::string maps_key = "maps"s;
const std::string roads_key = "roads"s;
const std::string buildings_key = "buildings"s;
const std::string offices_key = "offices"s;

const std::string id_key = "id"s;
const std::string name_key = "name"s;

const std::string x_key = "x"s;
const std::string y_key = "y"s;
const std::string x0_key = "x0"s;
const std::string y0_key = "y0"s;
const std::string x1_key = "x1"s;
const std::string y1_key = "y1"s;

const std::string width_key = "w"s;
const std::string height_key = "h"s;

const std::string offsetX_key = "offsetX"s;
const std::string offsetY_key = "offsetY"s;

const std::string code_key = "code"s;
const std::string message_key = "message"s;
} // namespace json_keys
