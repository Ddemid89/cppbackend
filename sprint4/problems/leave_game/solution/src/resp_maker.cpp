#include "resp_maker.h"

namespace resp_maker {
namespace beast = boost::beast;
namespace http  = beast::http;
namespace json  = boost::json;
namespace sys   = boost::system;
using namespace   std::literals;

namespace json_resp {
std::string GetBadRequestResponseBody(std::string message, std::string code) {
    using namespace json_keys;
    std::string_view code_, message_;
    if (code == ""sv) {
        code_ = bad_request_key;
    } else {
        code_ = code;
    }
    if (message == ""sv) {
        message_ = bad_request_message;
    } else {
        message_ = message;
    }

    json::value body = {{code_key, code_},{message_key, message_}};
    return json::serialize(body);
}

std::string GetNotFoundResponseBody(std::string message, std::string code) {
    using namespace json_keys;
    std::string_view code_, message_;

    if (code == ""sv) {
        code_ = not_found_key;
    } else {
        code_ = code;
    }
    if (message == ""sv) {
        message_ = not_found_message;
    } else {
        message_ = message;
    }

    json::value body = {{code_key, code_},{message_key, message_}};
    return json::serialize(body);
}
} // namespace json
} // namespace resp_maker
