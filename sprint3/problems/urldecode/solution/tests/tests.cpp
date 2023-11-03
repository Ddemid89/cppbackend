#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"
#include <algorithm>

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;
    std::string str_no_esc = "this/is/url/without/escape/sequence"s;
    std::string str_diff_cases = "%54%65%73%74%2e%2E%2e"s;
    std::string str_diff_res = "Test..."s;
    std::string str_invalid_1 = "JYkjbf%+0"s;
    std::string str_invalid_2 = "JYkjbf%pi"s;
    std::string str_invalid_3 = "JYkjbf%1g"s;
    std::string str_invalid_4 = "JYkjbf%o1"s;
    std::string str_part_esc = "JYkjbf%1"s;
    std::string str_pluses = "This is very long string whith some spaces."s;
    std::string url_pluses = str_pluses;
    std::replace(url_pluses.begin(), url_pluses.end(), ' ', '+');


    BOOST_TEST(UrlDecode(""sv) == ""s);
    BOOST_CHECK_EQUAL(UrlDecode(str_no_esc), str_no_esc);
    BOOST_CHECK_EQUAL(UrlDecode(str_diff_cases), str_diff_res);
    BOOST_CHECK_THROW(UrlDecode(str_invalid_1), std::logic_error);
    BOOST_CHECK_THROW(UrlDecode(str_invalid_2), std::logic_error);
    BOOST_CHECK_THROW(UrlDecode(str_invalid_3), std::logic_error);
    BOOST_CHECK_THROW(UrlDecode(str_invalid_4), std::logic_error);
    BOOST_CHECK_THROW(UrlDecode(str_part_esc), std::logic_error);
    BOOST_CHECK_EQUAL(UrlDecode(url_pluses), str_pluses);

    // Напишите остальные тесты для функции UrlDecode самостоятельно
}
