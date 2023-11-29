#pragma once

#include <string_view>

#include <boost/json.hpp>
#include <boost/beast.hpp>

#include "game_manager.h"
#include "resp_maker.h"
#include "model_serialization.h"
#include "move_manager.h"
#include "http_strs.h"

#include <optional>
#include <functional>
#include <memory>

#define BOOST_BEAST_USE_STD_STRING_VIEW

const size_t MAX_ITEMS = 100;

namespace game_manager {
namespace json = boost::json;
using namespace json_keys;

void tag_invoke (json::value_from_tag, json::value& jv, const PlayerInfo& pi);
void tag_invoke (json::value_from_tag, json::value& jv, const ItemInfo& info);

} // game_manager

namespace api_handler {

namespace beast = boost::beast;
namespace http = beast::http;
using namespace std::literals;
namespace json = boost::json;

struct RequestInfo {
    std::string target;
    std::string body;
    http::verb method;
    std::string content_type;
    int version;
    bool keep_alive;
    std::optional<game_manager::Token> auth;
    int start = 0;
    int max_number = 100;
};

enum class Method {
    post,
    get_head,
    any
};

class ApiHandler : public std::enable_shared_from_this<ApiHandler> {
public:
    ApiHandler(game_manager::GameManager& game);

    using ResponseInfo = resp_maker::detail::ResponseInfo;

    template <typename Body, typename Allocator, typename Send>
    void Handle(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send);
private:
    std::function<void(ResponseInfo)> send_;

    void HandleGameResponse();

    bool CheckEndPath();

    bool CheckMethod(Method method);

    bool CheckAuth();

    void HandleJoinRequest();

    void HandlePlayersStateRequest();

    void HandlePlayersListRequest();

    void HandleTickRequest();

    void HandleMoveRequest();

    void HandleRecordsRequest();

    json::value MakePlayersJson(const std::deque<game_manager::Player>& players);

    json::value MakeLootObjectsJson(const game_manager::LootObjectsContainer& objects);

    void HandleApiResponse();

    void HandleV1Response();

    void HandleMapsResponse();

    void HandleOneMapResponse();

    template <typename Body, typename Allocator>
    RequestInfo ParseRequest(const http::request<Body, http::basic_fields<Allocator>>& req);

    std::string FindAndCutTarget(RequestInfo& req);

    ResponseInfo MakeResponse(http::status status, bool no_cache);

    void SendOkResponse(const std::string& body, bool no_cache = true);

    void SendBadRequestResponse(std::string message, std::string code = json_keys::bad_request_key, bool no_cache = true);

    void SendBadRequestResponseDefault(bool no_cache = true) {
        SendBadRequestResponse(json_keys::bad_request_message, json_keys::bad_request_key, no_cache);
    }

    void SendNotFoundResponse(const std::string& message = json_keys::not_found_message,
                              const std::string& key = json_keys::not_found_key, bool no_cache = true);

    void SendNoAuthResponse(const std::string& message = json_keys::bad_token_mess,
                            const std::string& key = json_keys::bad_token_key, bool no_cache = true);

    void SendWrongMethodResponseAllowedGetHead (const std::string& message = json_keys::invalid_method_message_get_head,
                                      bool no_cache = true);

    void SendWrongMethodResponseAllowedPost (const std::string& message = json_keys::invalid_method_message_get_head,
                                      bool no_cache = true);

    void SetStartOrMaxNumber(RequestInfo& info, const std::string& key, int val);

    void ParseStartAndMaxNumber(RequestInfo& info);

    game_manager::GameManager& game_;
    RequestInfo req_info_;
};

template <typename Body, typename Allocator, typename Send>
void HandleApiRequest(game_manager::GameManager& game,
                      http::request<Body, http::basic_fields<Allocator>>&& req,
                      Send&& send);

} // namespace api_handler

//===================================Templates implementation============================================

namespace api_handler {

template <typename Body, typename Allocator>
RequestInfo ApiHandler::ParseRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
    RequestInfo result;
    result.target = req.target();
    result.body = req.body();
    result.method = req.method();
    result.version = req.version();
    result.keep_alive = req.keep_alive();

    ParseStartAndMaxNumber(result);

    if (req.find(http::field::content_type) != req.end()) {
        result.content_type = req.at(http::field::content_type);
    }

    auto it = req.find(http::field::authorization);
    if (it != req.end()) {
        std::string token_str{req.at(http::field::authorization)};
        if (token_str.size() <= json_keys::token_prefix.size()) {
            return result;
        }
        token_str = token_str.substr(json_keys::token_prefix.size());
        if (token_str.size() == game_.TOKEN_SIZE) {
            result.auth = game_manager::Token{token_str};
        }
    }

    return result;
}

template <typename Body, typename Allocator, typename Send>
void ApiHandler::Handle(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
    send_ = std::forward<Send>(send);
    req_info_ = ParseRequest(req);
    if (req_info_.target.starts_with(http_strs::game_path)) {
        req_info_.target = req_info_.target.substr(http_strs::game_path.size());
        HandleGameResponse();
    } else {
        req_info_.target = req_info_.target.substr(http_strs::api.size());
        HandleApiResponse();
    }
}

template <typename Body, typename Allocator, typename Send>
void HandleApiRequest(game_manager::GameManager& game,
                      http::request<Body, http::basic_fields<Allocator>>&& req,
                      Send&& send)
{
    auto handler = std::make_shared<ApiHandler>(game);

    handler->Handle(req, [send = std::forward<Send>(send)](const resp_maker::detail::ResponseInfo& info){
        send(resp_maker::detail::MakeTextResponse<Body, Allocator>(info));
    });
}

} // namespace api_handler
