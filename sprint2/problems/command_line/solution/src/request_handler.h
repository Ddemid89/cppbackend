#pragma once
#include "http_server.h"
#include "model.h"
#include "game_manager.h"
#include "api_handler.h"
#include "resp_maker.h"
#include "http_strs.h"

#include <vector>
#include <string_view>
#include <optional>
#include <filesystem>
#include <unordered_map>

#include <boost/json.hpp>
#include <boost/beast.hpp>

namespace json = boost::json;

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace sys = boost::system;
using HttpResponse = http::response<http::string_body>;
using namespace std::literals;
namespace net = boost::asio;

class RequestHandler {
public:
    explicit RequestHandler(game_manager::GameManager& game, std::filesystem::path static_data_path)
        : game_{game},
          static_data_path_(static_data_path) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

private:
    template <typename Body, typename Allocator, typename Send>
    void HandleStaticDataResponse(http::request<Body, http::basic_fields<Allocator>>&& req,
                               std::string_view target, Send&& send);

    template <typename Body, typename Allocator, typename Send>
    void TryToSendFile(http::request<Body, http::basic_fields<Allocator>>& req,
                       const std::filesystem::path& file, Send&& send);

    std::filesystem::path static_data_path_;
    game_manager::GameManager& game_;
};

}  // namespace http_handler

#include "request_handler.tpp"
