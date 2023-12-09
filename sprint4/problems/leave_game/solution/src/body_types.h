#pragma once

#include <string>

namespace body_type {
using namespace std::literals;

const std::string html = "text/html"s; //.htm, .html: text/html
const std::string css = "text/css"s; //.css: text/css
const std::string txt = "text/plain"s; //.txt: text/plain
const std::string js = "text/javascript"s; //.js: text/javascript
const std::string json = "application/json"s; //.json: application/json
const std::string xml = "application/xml"s; //.xml: application/xml
const std::string png = "image/png"s; //.png: image/png
const std::string jpg = "image/jpeg"s; //.jpg, .jpe, .jpeg: image/jpeg
const std::string gif = "image/gif"s; //.gif: image/gif
const std::string bmp = "image/bmp"s; //.bmp: image/bmp
const std::string ico = "image/vnd.microsoft.icon"s; //.ico: image/vnd.microsoft.icon
const std::string tif = "image/tiff"s; //.tiff, .tif: image/tiff
const std::string svg = "image/svg+xml"s; //.svg, .svgz: image/svg+xml
const std::string mp3 = "audio/mpeg"s; //.mp3: audio/mpeg
const std::string unknown = "application/octet-stream"s; //other

std::string to_lower_case(std::string_view str);
std::string_view GetTypeByExtention(std::string_view file);
} // namespace body_type
