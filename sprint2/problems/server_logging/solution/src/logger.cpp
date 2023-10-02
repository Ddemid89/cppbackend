#include "logger.h"

namespace json_logger {

namespace logging = boost::log;
namespace json = boost::json;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace sys = boost::system;
namespace net = boost::asio;
namespace http = boost::beast::http;

using tcp = net::ip::tcp;

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    json::value data = *rec[additional_data];
    auto ts = *rec[timestamp];
    auto message = *rec[expr::smessage];

    json::value result = {
        {"timestamp", to_iso_extended_string(ts)},
        {"data", data},
        {"message", message}
    };

    strm << result;
}


JsonLogger& JsonLogger::GetInstance() {
    static JsonLogger logger;
    return logger;
}

void JsonLogger::LogJson(std::string_view message, const json::value& data) {
    BOOST_LOG_TRIVIAL(trace) << logging::add_value(additional_data, data) << message;
}

void JsonLogger::LogError(std::string_view where, const sys::error_code& ec) {
    json::value data = {
        {"code", ec.value()},
        {"text", ec.message()},
        {"where", where}
    };
    LogJson("error", data);
}

void JsonLogger::LogServerStarted(const tcp::endpoint& ep) {
    json::value data = {
        {"port", ep.port()},
        {"address", ep.address().to_string()}
    };
    LogJson("server started", data);
}

void JsonLogger::LogServerNormalFinish() {
    json::value data = {
        {"code", 0}
    };
    LogJson("server exited", data);
}

void JsonLogger::LogServerErrorFinish(const std::exception& ec) {
    json::value data = {
        {"code", "EXIT_FAILURE"},
        {"exception", ec.what()}
    };
    LogJson("server exited", data);
}

void JsonLogger::LogRequest(std::string_view client_ip, std::string_view target, std::string_view method) {
    json::value data = {
        {"ip", client_ip},
        {"URI", target},
        {"method", method}
    };
    LogJson("request received", data);
}

void JsonLogger::LogResponse(std::chrono::steady_clock::duration dur, unsigned int code, std::string_view content_type) {
    using namespace std::chrono;
    json::value data = {
        {"response_time", duration_cast<milliseconds>(dur).count()},
        {"code", code},
        {"content_type", content_type}
    };
    LogJson("response sent", data);
}


JsonLogger::JsonLogger() {
    logging::add_common_attributes();
    logging::add_console_log(
        std::clog,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
    );
}


} // namespace json_logger
