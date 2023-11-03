#include "urldecode.h"

#include <charconv>
#include <stdexcept>


int IntFromHex(char hex) {
    if ('0' <= hex && hex <= '9') {
        return hex - '0';
    } else if ('A' <= hex && hex <= 'F') {
        return hex - 'A' + 10;
    } else if ('a' <= hex && hex <= 'f') {
        return hex - 'a' + 10;
    } else {
        return -1;
    }
}

int IntFrom2Hex(char first, char second) {
    int first_int  = IntFromHex(first);
    int second_int = IntFromHex(second);

    if (first_int < 0 || second_int < 0) {
        throw std::logic_error("Invalid hex number");
    }

    return IntFromHex(first) * 16 + IntFromHex(second);
}


std::string UrlDecode(std::string_view url) {
    size_t size = url.size();
    size_t i = 0;
    std::string result;
    result.reserve(url.size());

    while (i < size) {
        char cur = url.at(i);
        if (cur == '%') {
            if (i > size - 3) {
                throw std::invalid_argument("Incorrect url");
            }
            int hex = IntFrom2Hex(url.at(i + 1), url.at(i + 2));
            result.push_back(static_cast<char>(hex));
            i += 2;
        } else if (cur == '+') {
            result.push_back(' ');
        } else {
            result.push_back(cur);
        }
        i++;
    }

    return result;
}
