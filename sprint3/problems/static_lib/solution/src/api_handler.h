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

struct RequestInfo {
    std::string target;
    std::string body;
    http::verb method;
    std::string content_type;
    int version;
    bool keep_alive;
    std::optional<game_manager::Token> auth;
};

enum class Method {
    post,
    get_head,
    any
};

class ApiHandler2 : public std::enable_shared_from_this<ApiHandler2> {
public:
    ApiHandler2(game_manager::GameManager& game) : game_(game) {}

    using ResponseInfo = resp_maker::detail::ResponseInfo;

    template <typename Body, typename Allocator, typename Send>
    void Handle(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
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
private:
    std::function<void(ResponseInfo)> send_;

    void HandleGameResponse() {
        std::string path_part = FindAndCutTarget(req_info_);
        if (path_part == http_strs::join) {
            HandleJoinRequest();
        } else if (path_part == http_strs::players) {
            HandlePlayersListRequest();
        } else if (path_part == http_strs::state) {
            HandlePlayersStateRequest();
        } else if (path_part == http_strs::player) {
            HandleMoveRequest();
        } else if (path_part == http_strs::tick) {
            HandleTickRequest();
        } else {
            SendBadRequestResponseDefault();
        }
    }

    bool CheckEndPath() {
        if (req_info_.target == "/"sv || req_info_.target.empty()) {
            return true;
        }
        return false;
    }

    bool CheckMethod(Method method) {
        if (req_info_.method == http::verb::post && method == Method::post) {
            return true;
        } else if ((req_info_.method == http::verb::get || req_info_.method == http::verb::head) && method == Method::get_head) {
            return true;
        } else if (method == Method::any) {
            return true;
        } else {
            return false;
        }
    }

    bool CheckAuth() {
        if (req_info_.auth.has_value()) {
            if ((**req_info_.auth).size() == game_.TOKEN_SIZE) {
                return true;
            }
        }
        return false;
    }

    void HandleJoinRequest() {
        using namespace resp_maker;

        if (!CheckEndPath()) {
            SendBadRequestResponse("Unsupported request"s);
            return;
        }

        if (!CheckMethod(Method::post)) {
            SendWrongMethodResponseAllowedPost(json_keys::invalid_method_message_post);
            return;
        }

        json::value player = json::parse(std::string{req_info_.body});

        if (!player.as_object().contains(json_keys::user_name_key) || !player.as_object().contains(json_keys::map_id_key)) {
            SendBadRequestResponse("Invalid join request format", "invalidArgument");
            return;
        }

        std::string user_name;
        std::string map_id;

        user_name = player.at(json_keys::user_name_key).as_string();
        map_id    = player.at(json_keys::map_id_key).as_string();

        if (user_name.find_first_not_of(' ') == user_name.npos) {
            SendBadRequestResponse("Invalid name", "invalidArgument");
            return;
        }

        if (game_.FindMap(model::Map::Id{map_id}) == nullptr) {
            SendNotFoundResponse("Map not found");
            return;
        }

        game_.Join(std::move(user_name), model::Map::Id{map_id},
            [self = this->shared_from_this()]
            (game_manager::PlayerInfo info) {
                json::value player = json::value_from(info);
                std::string bd = json::serialize(player);
                self->SendOkResponse(bd);
            }
        );
    }

    void HandlePlayersStateRequest() {
        using namespace resp_maker::json_resp;

        if (!CheckEndPath()) {
            SendBadRequestResponse("Unsupported request"s);
            return;
        }

        if (!CheckMethod(Method::get_head)) {
            SendWrongMethodResponseAllowedGetHead(json_keys::invalid_method_message_get_head);
            return;
        }

        if (!CheckAuth()) {
            SendNoAuthResponse();
            return;
        }

        using namespace game_manager;
        game_.GetPlayers(*req_info_.auth,
          [self = this->shared_from_this()] (const std::optional<PlayersAndObjects>& players_and_objects, Result res) {
              if (res == Result::ok) {
                  json::object stat;
                  stat[json_keys::players_key] = self->MakePlayersJson(players_and_objects->players);
                  stat[json_keys::lost_objects_key] = self->MakeLootObjectsJson(players_and_objects->objects);
                  self->SendOkResponse(json::serialize(stat));
              } else if (res == Result::no_token) {
                  self->SendNoAuthResponse(json_keys::unknown_token_mess, json_keys::unknown_token_key);
              } else {
                  self->SendNotFoundResponse("Player`s session not found", "sessionNotFound");
              }
        });
    }

    void HandlePlayersListRequest() {
        using namespace resp_maker::json_resp;

        game_manager::Token token{""};

        if (!CheckEndPath()) {
            SendBadRequestResponse("Unsupported request"s);
            return;
        }

        if (!CheckMethod(Method::get_head)) {
            SendWrongMethodResponseAllowedGetHead(json_keys::invalid_method_message_get_head);
            return;
        }

        if (!CheckAuth()) {
            SendNoAuthResponse();
            return;
        }

        using namespace game_manager;
        game_.GetPlayers(*req_info_.auth,
        [self = this->shared_from_this()]
        (std::optional<PlayersAndObjects> players_and_objects, Result res)
        {
            using namespace resp_maker::json_resp;
            using namespace json_keys;
            if (res == Result::ok) {
                json::object obj;

                for (const Player& player : players_and_objects->players) {
                    obj.emplace(std::to_string(player.id), json::value{"name", player.name});
                }
                self->SendOkResponse(json::serialize(obj));
            } else if (res == Result::no_token) {
                self->SendNoAuthResponse(json_keys::unknown_token_mess, json_keys::unknown_token_key);
            } else {
                self->SendNotFoundResponse("Player`s session not found", "sessionNotFound");
            }
        });
    }

    void HandleTickRequest() {
        using namespace resp_maker;

        if (!game_.IsTestMode()) {
            SendBadRequestResponse("Invalid endpoint");
            return;
        }

        if (!CheckEndPath()) {
            SendBadRequestResponse("Unsupported request"s);
            return;
        }

        if (!CheckMethod(Method::post)) {
            SendWrongMethodResponseAllowedPost(json_keys::invalid_method_message_post);
            return;
        }

        json::value jv;

        try {
            jv = json::parse(req_info_.body);
        } catch (...) {
            SendBadRequestResponse("Failed to parse action", "invalidArgument");
            return;
        }

        if (!jv.is_object() || !jv.as_object().contains(json_keys::time_delta_key)) {
            SendBadRequestResponse("Failed to parse action", "invalidArgument");
            return;
        }

        uint64_t duration;

        boost::json::string dur_boost_str;

        json::value dur = jv.at(json_keys::time_delta_key);

        if (dur.is_double() || dur.if_bool()) {
            SendBadRequestResponse("Failed to parse action", "invalidArgument");
            return;
        }

        if (dur.if_string()) {
            try {
                dur_boost_str = jv.at(json_keys::time_delta_key).as_string();
            } catch (...) {
                SendBadRequestResponse("Failed to parse action", "invalidArgument");
                return;
            }
            std::string dur_str{dur_boost_str.data(), dur_boost_str.size()};

            if (dur_str.find_first_not_of("0123456789") != dur_str.npos || dur_str.empty()) {
                SendBadRequestResponse("Failed to parse action", "invalidArgument");
                return;
            }
            duration = std::stoi(dur_str);
        } else if (dur.is_int64()) {
            try {
                duration = dur.as_int64();
            } catch (...) {
                SendBadRequestResponse("Failed to parse action", "invalidArgument");
                return;
            }
        } else if (dur.is_uint64()) {
            try {
                duration = dur.as_uint64();
            } catch (...) {
                SendBadRequestResponse("Failed to parse action", "invalidArgument");
                return;
            }
        } else {
            SendBadRequestResponse("Failed to parse action", "invalidArgument");
            return;
        }

        if (duration == 0) {
            SendBadRequestResponse("Failed to parse action", "invalidArgument");
            return;
        }

        game_.CallTick(duration,
            [self = this->shared_from_this()](game_manager::Result res){
                self->SendOkResponse("{}");
            }
        );
    }

    void HandleMoveRequest() {
        using namespace resp_maker;

        std::string path_part = FindAndCutTarget(req_info_);
        if (path_part != http_strs::action) {
            SendBadRequestResponseDefault();
            return;
        }

        if (!CheckEndPath()) {
            SendBadRequestResponse("Unsupported request"s);
            return;
        }

        if (!CheckMethod(Method::post)) {
            SendWrongMethodResponseAllowedPost(json_keys::invalid_method_message_post);
            return;
        }

        json::value jv = json::parse(req_info_.body);


        if (!jv.is_object() || !jv.as_object().contains(json_keys::move_key)) {
            SendBadRequestResponse("Failed to parse action", "invalidArgument");
            return;
        }

        std::optional<move_manager::Direction> dir = move_manager::GetDirectionFromString(jv.at(json_keys::move_key).as_string());

        if (dir.has_value()) {
            game_.MovePlayer(*req_info_.auth, *dir,
                [self = this->shared_from_this()]
                (game_manager::Result res){
                    using namespace game_manager;
                    using namespace resp_maker::json_resp;
                    if (res == Result::ok) {
                        self->SendOkResponse("{}"s);
                    } else if (res == Result::no_token) {
                        self->SendNoAuthResponse(unknown_token_mess, unknown_token_key);
                    } else if (res == Result::no_session) {
                        self->SendNotFoundResponse("Player`s session not found", "sessionNotFound");
                    }
                }
            );
        } else {
            SendBadRequestResponse("Failed to parse action", "invalidArgument");
            return;
        }
    }


    json::value MakePlayersJson(const std::deque<game_manager::Player>& players) {
        using namespace game_manager;
        json::object obj;
        for (const Player& player : players) {
            const move_manager::State& state = player.state;
            json::value jv = {
                {json_keys::pos_key, json::array{state.position.coor.x, state.position.coor.y}},
                {json_keys::speed_key, json::array{state.speed.x_axis, state.speed.y_axis}},
                {json_keys::dir_key, move_manager::GetStringDirection(state.dir)}
            };
            obj.emplace(std::to_string(player.id), jv);
        }
        return obj;
    }

    json::value MakeLootObjectsJson(const game_manager::LootObjectsContainer& objects) {
        using namespace game_manager;
        json::object obj;
        for (const LootObject& object : objects) {
            move_manager::Coords coords = object.position.coor;
            json::value jv = {
                {json_keys::type_key, object.type},
                {json_keys::pos_key, json::array{coords.x, coords.y}}
            };
            obj.emplace(std::to_string(object.id), jv);
        }
        return obj;
    }


    void HandleApiResponse() {
        std::string path_part = FindAndCutTarget(req_info_);
        if (path_part == http_strs::v1) {
            HandleV1Response();
            return;
        } else {
            SendBadRequestResponseDefault();
            return;
        }
    }

    void HandleV1Response() {
        std::string path_part = FindAndCutTarget(req_info_);
        if (path_part == http_strs::maps) {
            HandleMapsResponse();
            return;
        } else {
            SendBadRequestResponseDefault();
            return;
        }
    }

    void HandleMapsResponse() {
        if (CheckEndPath()) {
            json::value jv = json::value_from(game_.GetMapsInfo());
            SendOkResponse(json::serialize(jv));
            return;
        } else {
            HandleOneMapResponse();
            return;
        }
    }

    void HandleOneMapResponse() {
        if (req_info_.target.find_last_of('/') != 0) {
            SendBadRequestResponseDefault();
            return;
        } else if (req_info_.method != http::verb::get && req_info_.method != http::verb::head) {
            SendWrongMethodResponseAllowedGetHead("Wrong method", true);
            return;
        } else {
            model::Map::Id map_id{req_info_.target.substr(1)};
            auto map_ptr = game_.FindMap(map_id);

            if (map_ptr == nullptr) {
                SendNotFoundResponse("Map not found");
                return;
            } else {
                json::value jv = json::value_from(*map_ptr);
                SendOkResponse(json::serialize(jv));
                return;
            }
        }
    }

    template <typename Body, typename Allocator>
    RequestInfo ParseRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
        RequestInfo result;
        result.target = req.target();
        result.body = req.body();
        result.method = req.method();
        result.version = req.version();
        result.keep_alive = req.keep_alive();

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

    std::string FindAndCutTarget(RequestInfo& req) {
        std::string res;
        size_t pos = req.target.find_first_of('/', 1);

        if (pos != req.target.npos) {
            res = req.target.substr(0, pos);
            req.target = req.target.substr(res.size());
            return res;
        }
        res = req.target;
        req.target = "";
        return res;
    }

    ResponseInfo MakeResponse(http::status status, bool no_cache) {
        ResponseInfo result;

        result.status = status;
        result.version = req_info_.version;
        result.content_type = body_type::json;
        result.keep_alive = req_info_.keep_alive;
        result.no_cache = no_cache;

        return result;
    }

    void SendOkResponse(const std::string& body, bool no_cache = true) {
        ResponseInfo result = MakeResponse(http::status::ok, no_cache);

        result.body = body;

        send_(result);
    }

    void SendBadRequestResponse(std::string message, std::string code = json_keys::bad_request_key, bool no_cache = true) {
        ResponseInfo result = MakeResponse(http::status::bad_request, no_cache);

        json::value body = {
            {json_keys::code_key, code},
            {json_keys::message_key, message}
        };

        result.body = json::serialize(body);

        send_(result);
    }

    void SendBadRequestResponseDefault(bool no_cache = true) {
        SendBadRequestResponse(json_keys::bad_request_message, json_keys::bad_request_key, no_cache);
    }

    void SendNotFoundResponse(const std::string& message = json_keys::not_found_message,
                              const std::string& key = json_keys::not_found_key, bool no_cache = true) {
        ResponseInfo result = MakeResponse(http::status::not_found, no_cache);

        json::value body = {
            {json_keys::code_key, key},
            {json_keys::message_key, message}
        };

        result.body = json::serialize(body);

        send_(result);
    }

    void SendNoAuthResponse(const std::string& message = json_keys::bad_token_mess,
                            const std::string& key = json_keys::bad_token_key, bool no_cache = true) {
        ResponseInfo result = MakeResponse(http::status::unauthorized, no_cache);

        json::value body = {
            {json_keys::code_key, key},
            {json_keys::message_key, message}
        };

        result.body = json::serialize(body);

        send_(result);
    }

    void SendWrongMethodResponseAllowedGetHead (const std::string& message = json_keys::invalid_method_message_get_head,
                                      bool no_cache = true) {
        ResponseInfo result = MakeResponse(http::status::method_not_allowed, no_cache);

        json::value body = {
            {json_keys::code_key, json_keys::invalid_method_key},
            {json_keys::message_key, message}
        };

        result.body = json::serialize(body);

        result.additional_fields.emplace_back(http::field::allow, "GET, HEAD"s);

        send_(result);
    }

    void SendWrongMethodResponseAllowedPost (const std::string& message = json_keys::invalid_method_message_get_head,
                                      bool no_cache = true) {
        ResponseInfo result = MakeResponse(http::status::method_not_allowed, no_cache);

        json::value body = {
            {json_keys::code_key, json_keys::invalid_method_key},
            {json_keys::message_key, message}
        };

        result.body = json::serialize(body);

        result.additional_fields.emplace_back(http::field::allow, "POST"s);

        send_(result);
    }

    game_manager::GameManager& game_;
    RequestInfo req_info_;
};


template <typename Body, typename Allocator>
class ApiHandlerWraper {
public:

    ApiHandlerWraper(game_manager::GameManager& game) : game_(game) {}

    template <typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto handler = std::make_shared<ApiHandler2>(game_);

        handler->Handle(req, [send = std::forward<Send>(send)] (const resp_maker::detail::ResponseInfo& info) {
            send(resp_maker::detail::MakeTextResponse<Body, Allocator>(info));
        });
    }
private:
    game_manager::GameManager& game_;
};

} // namespace api_handler



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

    ResponseInfo WrongResponseMethodInfo(std::string message = ""s, bool no_cache = false);

    ResponseInfo NotFoundInfo(std::string message = ""s, bool no_cache = false);

    template <typename Body, typename Allocator>
    RequestInfo MakeRequestInfo (http::request<Body, http::basic_fields<Allocator>>& req);

    std::string_view FindAndCutTarget(RequestInfo& req);

    game_manager::GameManager& game_;
};

} // namespace api_handler

//===================================Templates implementation============================================

namespace api_handler {
template <typename Body, typename Allocator, typename Send>
void ApiHandler::operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

    std::string_view target = req.target();
    using ResponseType = http::response<Body, http::basic_fields<Allocator>>;
    ResponseType response;
    ResponseInfo response_info;
    RequestInfo req_info = MakeRequestInfo(req);

    if (req_info.target.starts_with(http_strs::game_path)) {
        req_info.target = req_info.target.substr(http_strs::game_path.size());
        std::string_view path_part = FindAndCutTarget(req_info);
        if (path_part == http_strs::join) {
            return JoinResponse(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send));
        } else if (path_part == http_strs::players) {
            return PlayersList(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send));
        } else if (path_part == http_strs::state) {
            return PlayersState(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send));
        } else if (path_part == http_strs::player) {
            return MoveResponse(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send));
        } else if (path_part == http_strs::tick) {
            return TickResponse(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send));
        } else {
            response = resp_maker::json_resp::MakeBadRequestResponse(req);
        }
    } else {
        req_info.target = req_info.target.substr(http_strs::api.size());
        response_info = ApiResponse(req_info);
        response = resp_maker::detail::MakeTextResponse(req, response_info);
    }
        send(response);
        return;
}


template <typename Body, typename Allocator, typename Send>
void ApiHandler::JoinResponse(http::request<Body, http::basic_fields<Allocator>>&& req, RequestInfo& req_info, Send&& send) {
    using namespace resp_maker;

    json::value player;

    if(!CheckRequest(req, req_info, send, true, Method::post, std::nullopt, true, &player)) {
        return;
    }

    if (!player.as_object().contains(json_keys::user_name_key) || !player.as_object().contains(json_keys::map_id_key)) {
        send(json_resp::MakeBadRequestResponse(req, "Invalid join request format", "invalidArgument", true));
        return;
    }

    std::string user_name;
    std::string map_id;

    user_name = player.at(json_keys::user_name_key).as_string();
    map_id    = player.at(json_keys::map_id_key).as_string();

    if (user_name.find_first_not_of(' ') == user_name.npos) {
        send(json_resp::MakeBadRequestResponse(req, "Invalid name", "invalidArgument", true));
        return;
    }

    if (game_.FindMap(model::Map::Id{map_id}) == nullptr) {
        send(json_resp::MakeNotFoundResponse(req, "Map not found", true));
        return;
    }

    game_.Join(std::move(user_name), model::Map::Id{map_id},
        [send = std::forward<Send>(send), req = std::forward<decltype(req)>(req)]
        (game_manager::PlayerInfo info) {
            json::value player = json::value_from(info);
            std::string bd = json::serialize(player);

            send(resp_maker::json_resp::MakeOkResponse(req, json::serialize(player), true));
        }
    );
}

template <typename Body, typename Allocator, typename Send, typename Handler>
void ApiHandler::PlayersResponse(http::request<Body, http::basic_fields<Allocator>>&& req,
                     RequestInfo& req_info, Send&& send, Handler&& handler)
{
    game_manager::Token token{""};

    if (!CheckRequest(req, req_info, send, true, Method::get_head, &token, false, std::nullopt)) {
        return;
    }

    game_.GetPlayers(token, std::forward<Handler>(handler));
}

template <typename Body, typename Allocator, typename Send>
void ApiHandler::TickResponse(http::request<Body, http::basic_fields<Allocator>>&& req, RequestInfo& req_info, Send&& send)
{
    using namespace resp_maker;

    if (!game_.IsTestMode()) {
        send(json_resp::MakeBadRequestResponse(req, "Invalid endpoint", true));
        return;
    }

    json::value jv;

    if (!CheckRequest(req, req_info, send, true, Method::post, std::nullopt, true, &jv)) {
        return;
    }

    if (!jv.is_object() || !jv.as_object().contains(json_keys::time_delta_key)) {
        send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
        return;
    }

    uint64_t duration;

    boost::json::string dur_boost_str;

    json::value dur = jv.at(json_keys::time_delta_key);

    if (dur.is_double() || dur.if_bool()) {
        send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
        return;
    }

    if (dur.if_string()) {
        try {
            dur_boost_str = jv.at(json_keys::time_delta_key).as_string();
        } catch (...) {
            send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
            return;
        }
        std::string dur_str{dur_boost_str.data(), dur_boost_str.size()};

        if (dur_str.find_first_not_of("0123456789") != dur_str.npos) {
            send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
            return;
        }
        duration = std::stoi(dur_str);
    } else if (dur.is_int64()) {
        try {
            duration = dur.as_int64();
        } catch (...) {
            send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
            return;
        }
    } else if (dur.is_uint64()) {
        try {
            duration = dur.as_uint64();
        } catch (...) {
            send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
            return;
        }
    } else {
        send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
        return;
    }

    if (duration == 0) {
        send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
        return;
    }

    game_.CallTick(duration,
        [req = std::forward<decltype(req)>(req), send = std::forward<Send>(send)](game_manager::Result res){
            send(json_resp::MakeOkResponse(req, "{}"s, true));
        }
    );
}

template <typename Body, typename Allocator, typename Send>
void ApiHandler::MoveResponse(http::request<Body, http::basic_fields<Allocator>>&& req,
                     RequestInfo& req_info, Send&& send)
{
    using namespace resp_maker;

    std::string_view path_part = FindAndCutTarget(req_info);
    if (path_part != http_strs::action) {
        send(json_resp::MakeBadRequestResponse(req));
        return;
    }

    game_manager::Token token{""};

    json::value jv;

    if (!CheckRequest(req, req_info, send, true, Method::post, &token, true, &jv)) {
        return;
    }


    if (!jv.is_object() || !jv.as_object().contains(json_keys::move_key)) {
        send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
        return;
    }

    std::optional<move_manager::Direction> dir = move_manager::GetDirectionFromString(jv.at(json_keys::move_key).as_string());

    if (dir.has_value()) {
        game_.MovePlayer(std::move(token), *dir,
            [req = std::forward<decltype(req)>(req), send = std::forward<Send>(send)]
            (game_manager::Result res){
                using namespace game_manager;
                using namespace resp_maker::json_resp;
                if (res == Result::ok) {
                    send(MakeOkResponse(req, "{}"s, true));
                } else if (res == Result::no_token) {
                    send(MakeUnauthorizedResponse(req, unknown_token_mess, unknown_token_key, true));
                } else if (res == Result::no_session) {
                    send(MakeNotFoundResponse(req, "Player`s session not found", "sessionNotFound", true));
                } else {
                    send(MakeUnknownErrorResponse(req, "Unknown error in Players lambda", "unknownError", true));
                }
            }
        );
    } else {
        send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
        return;
    }
}

template <typename Body, typename Allocator, typename Send>
bool ApiHandler::CheckRequest (http::request<Body, http::basic_fields<Allocator>>& req,
                   RequestInfo& req_info, Send&& send, bool end_path, Method method,
                   std::optional<game_manager::Token*> token_out, bool json, std::optional<json::value*> value_out) {
    using namespace resp_maker::json_resp;
    if (end_path && req_info.target != "/"sv && !req_info.target.empty()) {
        send(MakeBadRequestResponse(req, "Unsupported request", true));
        return false;
    }
    if (method == Method::post && req_info.method != http::verb::post) {
        send(MakeMethodNotAllowedResponse(req, "POST", "", true));
        return false;
    } else if (method == Method::get_head && req_info.method != http::verb::get
               && req_info.method != http::verb::head) {
        send(MakeMethodNotAllowedResponse(req, "GET, HEAD", "Invalid method", true));
        return false;
    }

    if (token_out.has_value()) {
        auto it = req.find(http::field::authorization);

        if (it == req.end()) {
            send(MakeUnauthorizedResponse(req, json_keys::invalid_token_mess, json_keys::invalid_token_key, true));
            return false;
        }

        std::string token_str{req.at(http::field::authorization)};

        if (token_str.size() != game_.TOKEN_SIZE + json_keys::token_prefix.size()) {
            send(MakeUnauthorizedResponse(req, json_keys::invalid_token_mess, json_keys::invalid_token_key, true));
            return false;
        }

        token_str = token_str.substr(json_keys::token_prefix.size());

        **token_out = game_manager::Token{token_str};
    }

    if (json) {
        if (req_info.content_type != body_type::json) {
            send(MakeBadRequestResponse(req, "Unsupported format", true));
            return false;
        }
    }

    if (value_out.has_value()) {
        try {
            **value_out = json::parse(req_info.body);
        } catch (...) {
            send(MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
            return false;
        }
    }
    return true;
}

template <typename Body, typename Allocator, typename Send>
void ApiHandler::PlayersList(http::request<Body, http::basic_fields<Allocator>>&& req,
                     RequestInfo& req_info, Send&& send)
{
    using namespace resp_maker::json_resp;

    game_manager::Token token{""};

    if (!CheckRequest(req, req_info, send, true, Method::get_head, &token, false, std::nullopt)) {
        return;
    }

    using namespace game_manager;
    game_.GetPlayers(std::move(token),
    [send = std::forward<Send>(send), req = std::forward<decltype(req)>(req)]
    (std::optional<std::deque<Player>> players, Result res)
    {
        using namespace resp_maker::json_resp;
        using namespace json_keys;
        if (res == Result::ok) {
            json::object obj;

            for (const Player& player : *players) {
                obj.emplace(std::to_string(player.id), json::value{"name", player.name});
            }

            send(MakeOkResponse(req, json::serialize(obj), true));
        } else if (res == Result::no_token) {
            send(MakeUnauthorizedResponse(req, unknown_token_mess, unknown_token_key, true));
        } else if (res == Result::no_session) {
            send(MakeNotFoundResponse(req, "Player`s session not found", "sessionNotFound", true));
        } else {
            send(MakeUnknownErrorResponse(req, "Unknown error in Players lambda", "unknownError", true));
    }
    });
}

template <typename Body, typename Allocator, typename Send>
void ApiHandler::PlayersState(http::request<Body, http::basic_fields<Allocator>>&& req,
                     RequestInfo& req_info, Send&& send)
{
    using namespace resp_maker::json_resp;

    game_manager::Token token{""};

    if (!CheckRequest(req, req_info, send, true, Method::get_head, &token, false, std::nullopt)) {
        return;
    }

    using namespace game_manager;
    game_.GetPlayers(std::move(token),
    [send = std::forward<Send>(send), req = std::forward<decltype(req)>(req)]
    (std::optional<std::deque<Player>> players, Result res)
    {
        using namespace resp_maker::json_resp;
        using namespace json_keys;
        if (res == Result::ok) {
            json::object obj;

            for (const Player& player : *players) {
                const move_manager::State& state = player.state;
                json::value jv = {
                    {json_keys::pos_key, json::array{state.position.coor.x, state.position.coor.y}},
                    {json_keys::speed_key, json::array{state.speed.x_axis, state.speed.y_axis}},
                    {json_keys::dir_key, move_manager::GetStringDirection(state.dir)}
                };
                obj.emplace(std::to_string(player.id), jv);
            }

            json::object res;
            res[json_keys::players_key] = obj;

            send(MakeOkResponse(req, json::serialize(res), true));
        } else if (res == Result::no_token) {
            send(MakeUnauthorizedResponse(req, unknown_token_mess, unknown_token_key, true));
        } else if (res == Result::no_session) {
            send(MakeNotFoundResponse(req, "Player`s session not found", "sessionNotFound", true));
        } else {
            send(MakeUnknownErrorResponse(req, "Unknown error in Players lambda", "unknownError", true));
        }
    });
}

template <typename Body, typename Allocator>
RequestInfo ApiHandler::MakeRequestInfo (http::request<Body, http::basic_fields<Allocator>>& req) {
    RequestInfo result;
    result.target = req.target();
    result.body = req.body();
    result.method = req.method();

    if (req.find(http::field::content_type) != req.end()) {
        result.content_type = req.at(http::field::content_type);
    }

    return result;
}

} // namespace api_handler

