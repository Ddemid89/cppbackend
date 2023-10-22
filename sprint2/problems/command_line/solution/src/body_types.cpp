#include "body_types.h"
#include <unordered_map>

namespace body_type {
namespace detail {
std::string GetExtention(std::string_view file_name) {
    size_t last_dot = file_name.find_last_of('.');
    if (last_dot == file_name.npos || last_dot == file_name.size() - 1) {
        return "";
    }
    return to_lower_case(file_name.substr(last_dot + 1));
}
} // namespace detail


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

std::string_view GetTypeByExtention(std::string_view file) {
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

    std::string extention = detail::GetExtention(file);
    auto it = types.find(extention);

    if (it == types.end()) {
        return unknown;
    } else {
        return *it->second;
    }
}
} // namespace body_type
