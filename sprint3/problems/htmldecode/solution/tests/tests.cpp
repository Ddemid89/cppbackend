#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Text without mnemonics", "[HtmlDecode]") {
    std::string without  = "This is &string lt without&Gt html &mnemonics"s;
    std::string with     = "This is &LTstring&gt; &apos;with&APOS &amphtml &quotmnemonics&QUOT;"s;
    std::string with_res = "This is <string> 'with' &html \"mnemonics\""s;

    std::string strt_mid_fin     = "&quot;begin&AMPend&QUOT;"s;
    std::string strt_mid_fin_res = "\"begin&end\""s;

    std::string part = "This is &quoparts&apo; of &lmnemonics"s;

    CHECK(HtmlDecode(""sv) == ""s);
    CHECK(HtmlDecode("hello"sv) == "hello"s);
    CHECK(HtmlDecode(without) == without);
    CHECK(HtmlDecode(with) == with_res);
    CHECK(HtmlDecode(strt_mid_fin) == strt_mid_fin_res);
    CHECK(HtmlDecode(part) == part);

}

// Напишите недостающие тесты самостоятельно
