#pragma once

#include <string_view>
#include <vector>
#include <utility>
#include <filesystem>

//#include <iostream>


#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <boost/system.hpp>

#include "body_types.h"
#include "json_keys.h"


namespace resp_maker {
namespace beast = boost::beast;
namespace http  = beast::http;
namespace json  = boost::json;
namespace sys   = boost::system;
using namespace   std::literals;

namespace detail {

struct ResponseInfo {
    http::status status;
    std::string body;
    std::string content_type;
    bool no_cache = false;
    std::vector<std::pair<http::field, std::string>> additional_fields;
};

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeTextResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                 ResponseInfo& resp_info);
} // namespace detail

namespace file_resp {
template <typename Body, typename Allocator>
http::response<http::file_body, http::basic_fields<Allocator>>
MakeFileResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                 const std::filesystem::path& file);
} // namespace file_resp

namespace txt_resp {
template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeBadRequestResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache = false);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeNotFoundResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache = false);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeServerErrorResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache = false);
} // namespace txt_resp

namespace json_resp {
std::string GetBadRequestResponseBody(std::string message = "", std::string code = "");

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeBadRequestResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message = ""s, std::string code = ""s, bool no_cache = false);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeBadRequestResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache = false);

std::string GetNotFoundResponseBody(std::string message = ""s, std::string code = ""s);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeNotFoundResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message = ""s, std::string code = ""s, bool no_cache = false);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeNotFoundResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache) {
    return MakeNotFoundResponse(req, std::move(message), ""s, no_cache);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeOkResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string body, bool no_cache = false);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeServerErrorResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string body, bool no_cache = false);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeMethodNotAllowedResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string allowed, std::string message = ""s, bool no_cache = false);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeUnauthorizedResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message = ""s, std::string code = ""s, bool no_cache = false);

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeUnknownErrorResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, std::string code, bool no_cache = false);


} // namespace json
} // namespace resp_maker

#include "resp_maker.tpp"
