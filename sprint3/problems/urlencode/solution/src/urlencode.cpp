#include "urlencode.h"
#include <optional>
#include <array>


using namespace std::literals;

const std::array<char, 16> nums = {'0', '1', '2', '3', '4',
                                   '5', '6', '7', '8', '9',
                                   'A', 'B', 'C', 'D', 'E', 'F'};

std::string IntToHex(int n) {
    char n1 = nums[n / 16];
    char n2 = nums[n % 16];
    return {'%', n1, n2};
}

std::optional<std::string> CheckSymbol(unsigned char c) {
    if (c == ' ') {
        return "+"s;
    }

    if (c < 31 || 127 < c) {
        return IntToHex(c);
    }

    size_t pos = "!#$&'()*+,/:;=?@[]"s.find_first_of(c);
    if (pos != std::string::npos) {
        return IntToHex(c);
    }

    return std::nullopt;
}


std::string UrlEncode(std::string_view str) {
    // Напишите реализацию самостоятельно
    std::string res;

    for (unsigned char c : str) {
        auto c2 = CheckSymbol(c);
        if (c2.has_value()) {
            res.append(*c2);
        } else {
            res.push_back(c);
        }
    }

    return res;
}
