#pragma once

#include <string_view>

#include <boost/json.hpp>
#include <boost/beast.hpp>

#include "game_manager.h"
#include "resp_maker.h"
#include "model_serialization.h"
#include "move_manager.h"
#include "http_strs.h"

#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace game_manager {
namespace json = boost::json;
using namespace json_keys;

void tag_invoke (json::value_from_tag, json::value& jv, const PlayerInfo& pi);

} // game_manager

namespace api_handler {

namespace beast = boost::beast;
namespace http = beast::http;
using namespace std::literals;
namespace json = boost::json;

class ApiHandler {

enum class ResultType {
    bad_request,
    ok,
    not_found,
    server_error,
    no_send
};

using ResponseInfo = resp_maker::detail::ResponseInfo;

struct RequestInfo {
    std::string_view target;
    std::string_view body;
    http::verb method;
    std::string_view content_type;
};

enum class Method {
    post,
    get_head,
    any
};

public:
    ApiHandler(game_manager::GameManager& game) : game_(game) {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

private:
    ResponseInfo ApiResponse(RequestInfo& req);

    ResponseInfo V1Response(RequestInfo& req);

    ResponseInfo MapsResponse(RequestInfo& req);

    ResponseInfo OneMapResponse(RequestInfo& req);

    template <typename Body, typename Allocator, typename Send>
    void JoinResponse(http::request<Body, http::basic_fields<Allocator>>&& req, RequestInfo& req_info, Send&& send);

    template <typename Body, typename Allocator, typename Send, typename Handler>
    void PlayersResponse(http::request<Body, http::basic_fields<Allocator>>&& req,
                         RequestInfo& req_info, Send&& send, Handler&& handler);

    template <typename Body, typename Allocator, typename Send>
    void PlayersList(http::request<Body, http::basic_fields<Allocator>>&& req,
                         RequestInfo& req_info, Send&& send);

    template <typename Body, typename Allocator, typename Send>
    void PlayersState(http::request<Body, http::basic_fields<Allocator>>&& req,
                         RequestInfo& req_info, Send&& send);

    template <typename Body, typename Allocator, typename Send>
    void MoveResponse(http::request<Body, http::basic_fields<Allocator>>&& req,
                         RequestInfo& req_info, Send&& send);

    template <typename Body, typename Allocator, typename Send>
    void TickResponse(http::request<Body, http::basic_fields<Allocator>>&& req, RequestInfo& req_info, Send&& send);

    template <typename Body, typename Allocator, typename Send>
    bool CheckRequest (http::request<Body, http::basic_fields<Allocator>>& req,
                       RequestInfo& req_info, Send&& send, bool end_path, Method method,
                       std::optional<game_manager::Token*> token_out, bool json,
                       std::optional<json::value*> value_out);

    ResponseInfo BadResponseInfo(std::string message = ""s, bool no_cache = false);

    ResponseInfo NotFoundInfo(std::string message = ""s, bool no_cache = false);

    template <typename Body, typename Allocator>
    RequestInfo MakeRequestInfo (http::request<Body, http::basic_fields<Allocator>>& req);

    bool FindAndCutTarget(RequestInfo& req, std::string_view part);

    game_manager::GameManager& game_;
};

} // namespace api_handler


#include "api_handler.tpp"
