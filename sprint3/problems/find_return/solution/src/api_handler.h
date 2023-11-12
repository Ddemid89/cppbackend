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
                {json_keys::dir_key, move_manager::GetStringDirection(state.dir)},
                {json_keys::bag_key, player.items_in_bag},
                {json_keys::score_key, player.score}
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
