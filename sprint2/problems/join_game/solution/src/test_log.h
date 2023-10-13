#pragma once

#include <boost/log/trivial.hpp>
#include <string>

#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр

#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/json.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace test_log {

namespace logging = boost::log;
namespace json = boost::json;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime);
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value);

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    //json::value data = *rec[additional_data];
    auto ts = *rec[timestamp];
    auto message = *rec[expr::smessage];

    //-----------------------------------------------------------------
    json::value data;
    auto data_ref = rec[additional_data];
    if (!data_ref.empty()) {
        data = *data_ref;
    } else {
        data = {"data", "no data yet"};
    }
    //-----------------------------------------------------------------

    json::value result = {
        {"timestamp", to_iso_extended_string(ts)},
        {"data", data},
        {"message", message}
    };

    strm << result;
}

void init() {
    logging::add_common_attributes();
    logging::add_console_log(
        std::clog,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
    );
}

void log(const std::string& text) {
    json::value data = {{"ak", "av"},
                        {{"bk", {{"ck", 1}, {"dk", 2}}}}};
    BOOST_LOG_TRIVIAL(trace) << logging::add_value(additional_data, data) << text;
}

void test() {
    init();
    log("Log1");
    log("Log2");

}

} // namespace test_log
