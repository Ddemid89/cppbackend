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

//===================================Templates implementation============================================

namespace http_handler {
    template <typename Body, typename Allocator, typename Send>
    void RequestHandler::operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        std::string_view target = req.target();

        if (target.starts_with(http_strs::api)) {
            api_handler::ApiHandlerWraper<Body, Allocator> api_h(game_);
            api_h(std::forward<decltype(req)>(req), std::forward<Send>(send));
            //api_handler::ApiHandler api_h(game_);
            //api_h(std::forward<decltype(req)>(req), std::forward<Send>(send));
        } else {
            HandleStaticDataResponse(std::forward<decltype(req)>(req), target, std::forward<Send>(send));
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void RequestHandler::HandleStaticDataResponse(http::request<Body, http::basic_fields<Allocator>>&& req,
                               std::string_view target, Send&& send)
    {
        using namespace resp_maker::txt_resp;
        namespace fs = std::filesystem;
        fs::path file = fs::weakly_canonical(static_data_path_ / target.substr(1));

        if (!file.string().starts_with(static_data_path_.string())) {
            return send(MakeBadRequestResponse(req, "Access denied"));
        }

        if (fs::exists(file)) {
            if (fs::is_directory(file)) {
                if (fs::exists(file / http_strs::index_html)) {
                    file /= http_strs::index_html;
                    TryToSendFile(req, file, send);
                } else if (fs::exists(file / http_strs::index_htm)){
                    file /= http_strs::index_htm;
                    TryToSendFile(req, file, send);
                } else {
                    return send(MakeNotFoundResponse(req, "No index.html in derictory"));
                }
            } else {
                TryToSendFile(req, file, send);
            }
        } else {
            return send(MakeNotFoundResponse(req, "No such file or directory"));
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void RequestHandler::TryToSendFile(http::request<Body, http::basic_fields<Allocator>>& req,
                       const std::filesystem::path& file, Send&& send)
    {
        using namespace resp_maker;
        try {
            return send(file_resp::MakeFileResponse(req, file));
        } catch (std::runtime_error& e) {
            return send(txt_resp::MakeServerErrorResponse(req, e.what()));
        } catch (...) {
            return send(txt_resp::MakeServerErrorResponse(req, "Unknown error"));
        }
    }

}  // namespace http_handler

