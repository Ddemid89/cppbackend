#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/log/trivial.hpp>
#include <string>
#include <chrono>
#include <string_view>

#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/json.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime);
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value);

namespace json_logger {

namespace logging = boost::log;
namespace json = boost::json;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace sys = boost::system;
namespace net = boost::asio;
namespace http = boost::beast::http;

using tcp = net::ip::tcp;

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

class JsonLogger {
public:
    static JsonLogger& GetInstance();
    void LogJson(std::string_view message, const json::value& data);
    void LogError(std::string_view where, const sys::error_code& ec);
    void LogServerStarted(const tcp::endpoint& ep);
    void LogServerNormalFinish();
    void LogServerErrorFinish(const std::exception& ec);
    void LogRequest(std::string_view client_ip, std::string_view target, std::string_view method);
    void LogResponse(std::chrono::steady_clock::duration dur, unsigned int code, std::string_view content_type);
private:
    JsonLogger();
};

} // namespace json_logger
