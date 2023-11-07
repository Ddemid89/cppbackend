#include "htmldecode.h"
#include <array>

struct Res {
    bool contains;
    char res;
    size_t size;
};

const int MNEMS = 5;

const std::array<std::string, MNEMS * 2> mnems = {"&lt",   "&LT"
                                                 ,"&gt",   "&GT"
                                                 ,"&amp",  "&AMP"
                                                 ,"&apos", "&APOS"
                                                 ,"&quot", "&QUOT"};

const std::array<char, MNEMS> chars = {'<'
                                      ,'>'
                                      ,'&'
                                      ,'\''
                                      ,'"'};

int GetCase(unsigned char c) {
    if ('a' <= c && c <= 'z') {
        return -1;
    } else if ('A' <= c && c <= 'Z') {
        return 1;
    }
    return 0;
}

bool CompStr(std::string_view where, std::string_view what) {
    if (what.size() > where.size()) {
        return false;
    }

    bool ok = true;

    for (size_t i = 0; i < what.size(); ++i) {
        ok = ok && (where.at(i) == what.at(i));
    }
    return ok;
}

Res SearchMnem(std::string_view str) {
    if (str.size() < 3) {
        return {false, 0, 0};
    }
    int let_case = GetCase(str.at(1));
    if (let_case == 0) {
        return {false, 0, 0};
    }
    let_case = (let_case + 1) / 2;

    for (int i = 0; i < MNEMS; ++i) {
        std::string_view mnem = mnems.at(i * 2 + let_case);
        if (CompStr(str, mnem)) {
            if (str.size() > mnem.size() && str.at(mnem.size()) == ';') {
                return {true, chars.at(i), mnem.size() + 1};
            } else {
                return {true, chars.at(i), mnem.size()};
            }
        }
    }
    return {false, 0, 0};
}

std::string HtmlDecode(std::string_view str) {
    std::string res;

    bool mnem = false;
    std::string part;
    int letter_case = 0;

    for (size_t i = 0; i < str.size(); ++i) {
        char c = str.at(i);

        if (c == '&') {
            auto mnem = SearchMnem(str.substr(i));
            if (mnem.contains == true) {
                res.push_back(mnem.res);
                i += mnem.size - 1;
            } else {
                res.push_back('&');
            }
        } else {
            res.push_back(c);
        }

    }

    return res;
}
