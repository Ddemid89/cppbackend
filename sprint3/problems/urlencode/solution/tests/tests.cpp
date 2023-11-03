#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    std::string no_esc = "NoEscapeSymbols"s;

    std::string esc    = "This_(string)/text_contains+symbols[@]:;&'*'?#?$?=,!"s;
    std::string esc_res = "This_%28string%29%2Ftext_contains%2Bsymbols%5B%40%5D%3A%3B%26%27%2A%27%3F%23%3F%24%3F%3D%2C%21"s;

    std::string str_spaces = "This is string with spaces"s;
    std::string str_spaces_res = "This+is+string+with+spaces"s;

    std::string str_spec = "Tabulation\tnew_line\nКириллица_имеет_коды_больше_ста_двадцати_восьмиЪ128"s;
    std::string str_spec_res = "Tabulation%09new_line%0A%D0%9A%D0%B8%D1%80%D0%B8%D0%BB%D0%BB%D0%B8%D1%86%D0%B0_%D0%B8%D0%BC%D0%B5%D0%B5%D1%82_%D0%BA%D0%BE%D0%B4%D1%8B_%D0%B1%D0%BE%D0%BB%D1%8C%D1%88%D0%B5_%D1%81%D1%82%D0%B0_%D0%B4%D0%B2%D0%B0%D0%B4%D1%86%D0%B0%D1%82%D0%B8_%D0%B2%D0%BE%D1%81%D1%8C%D0%BC%D0%B8%D0%AA128"s;

    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
    EXPECT_EQ(UrlEncode(""sv), ""s);
    EXPECT_EQ(UrlEncode(no_esc), no_esc);
    EXPECT_EQ(UrlEncode(esc), esc_res);
    EXPECT_EQ(UrlEncode(str_spaces), str_spaces_res);
    EXPECT_EQ(UrlEncode(str_spec), str_spec_res);
}

/* Напишите остальные тесты самостоятельно */
