#pragma once

#include <string_view>

#include <boost/json.hpp>
#include <boost/beast.hpp>

#include "game_manager.h"
#include "resp_maker.h"
#include "model_serialization.h"

#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace model {
namespace json = boost::json;

void tag_invoke (json::value_from_tag, json::value& jv, const Road& road);

void tag_invoke (json::value_from_tag, json::value& jv, const Building& building);

void tag_invoke (json::value_from_tag, json::value& jv, const Office& office);

void tag_invoke (json::value_from_tag, json::value& jv, const Map& map);

void tag_invoke (json::value_from_tag, json::value& jv, const MapInfo& map);
} // namespace model

namespace game_manager {
namespace json = boost::json;
using namespace json_keys;

void tag_invoke (json::value_from_tag, json::value& jv, const PlayerInfo& pi) {
    jv = {
        {auth_token_key, pi.token},
        {player_id_key, pi.Id}
    };
}

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

public:
    ApiHandler(game_manager::GameManager& game) : game_(game) {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

        std::string_view target = req.target();
        using ResponseType = http::response<Body, http::basic_fields<Allocator>>;
        ResponseType response;
        ResponseInfo response_info;
        RequestInfo req_info = MakeRequestInfo(req);


        if (FindAndCutTarget(req_info, "/api/v1/game"sv)) {
            if (FindAndCutTarget(req_info, "/join"sv)) {
                return JoinResponse(req, req_info, send);
            } else if (FindAndCutTarget(req_info, "/players"sv)) {
                return PlayersList(req, req_info, send);
            } else if (FindAndCutTarget(req_info, "/state"sv)) {
                return PlayersState(req, req_info, send);
            } else {
                response = resp_maker::json_resp::MakeBadRequestResponse(req);
            }
        } else {
            req_info.target = req_info.target.substr("/api"sv.size());
            response_info = ApiResponse(req_info);
            response = resp_maker::detail::MakeTextResponse(req, response_info);
        }
        return send(response);
    }

private:

    template <typename Body, typename Allocator, typename Send>
    void JoinResponse(http::request<Body, http::basic_fields<Allocator>>& req, RequestInfo& req_info, Send&& send) {
        using namespace resp_maker;

        if (req_info.target != "/"sv && !req_info.target.empty()) {
            return send(json_resp::MakeBadRequestResponse(req, "Unsupported request", true));
        }
        if (req_info.method != http::verb::post) {
            return send(json_resp::MakeMethodNotAllowedResponse(req, "POST", "", true));
        }
        if (req_info.content_type != body_type::json) {
            return send(json_resp::MakeBadRequestResponse(req, "Unsupported format", true));
        }

        json::value player;
        try {
            player = json::parse(std::string(req_info.body));
        } catch (...) {
            return send(json_resp::MakeBadRequestResponse(req, "Join game request parse error", "invalidArgument", true));
        }

        if (!player.as_object().contains("userName"s) || !player.as_object().contains("mapId"s)) {
            return send(json_resp::MakeBadRequestResponse(req, "Invalid join request format", "invalidArgument", true));
        }

        std::string user_name;
        std::string map_id;

        user_name = player.at("userName"s).as_string();
        map_id    = player.at("mapId"s).as_string();

        if (user_name.find_first_not_of(' ') == user_name.npos) {
            return send(json_resp::MakeBadRequestResponse(req, "Invalid name", "invalidArgument", true));
        }

        if (game_.FindMap(model::Map::Id{map_id}) == nullptr) {
            return send(json_resp::MakeNotFoundResponse(req, "Map not found", true));
        }

        //Trim name???

        game_.Join(std::move(user_name), model::Map::Id{map_id},
            [send = std::move(send), req = std::move(req)]
            (std::optional<game_manager::PlayerInfo> info) {
                json::value player = json::value_from(*info);
                std::string bd = json::serialize(player);

                send(resp_maker::json_resp::MakeOkResponse(req, json::serialize(player), true));
            }
        );
    }

    ResponseInfo ApiResponse(RequestInfo& req) {
        if (FindAndCutTarget(req, "/v1"sv)) {
            return V1Response(req);
        } else {
            return BadResponseInfo();
        }
    }

    ResponseInfo V1Response(RequestInfo& req) {
        if (FindAndCutTarget(req, "/maps"sv)) {
            return MapsResponse(req);
        } else {
            return BadResponseInfo();
        }
    }

    ResponseInfo MapsResponse(RequestInfo& req) {
        if (req.target == "/"sv || req.target.empty()) {
            json::value jv = json::value_from(game_.GetMapsInfo());
            return {http::status::ok, json::serialize(jv), body_type::json};
        } else {
            return OneMapResponse(req);
        }
    }

    ResponseInfo OneMapResponse(RequestInfo& req) {
        if (req.target.find_last_of('/') != 0) {
            return BadResponseInfo();
        } else {
            model::Map::Id map_id{std::string(req.target.substr(1))};
            auto map_ptr = game_.FindMap(map_id);

            if (map_ptr == nullptr) {
                return BadResponseInfo();
            } else {
                json::value jv = json::value_from(*map_ptr);
                return {http::status::ok, json::serialize(jv), body_type::json};
            }
        }
    }    

    template <typename Body, typename Allocator, typename Send, typename Handler>
    void PlayersResponse(http::request<Body, http::basic_fields<Allocator>>& req,
                         RequestInfo& req_info, Send&& send, Handler&& handler) {
        using namespace resp_maker::json_resp;

        static size_t token_size = 32;

        if (req_info.target != "/"sv && !req_info.target.empty()) {
            return send(MakeBadRequestResponse(req, "Unsupported request", true));
        }

        if (req_info.method != http::verb::get && req_info.method != http::verb::head) {
            return send(MakeMethodNotAllowedResponse(req, "GET, HEAD", "Invalid method", true));
        }

        auto it = req.find(http::field::authorization);

        if (it == req.end()) {
            return send(MakeUnauthorizedResponse(req, json_keys::invalid_token_mess, json_keys::invalid_token_key, true));
        }

        std::string token_str{req.at(http::field::authorization)};

        if (token_str.size() != token_size + json_keys::token_prefix.size()) {
            return send(MakeUnauthorizedResponse(req, json_keys::invalid_token_mess, json_keys::invalid_token_key, true));
        }

        token_str = token_str.substr(json_keys::token_prefix.size());

        game_manager::Token token{token_str};

        using namespace game_manager;

        game_.GetPlayers(token, std::forward<Handler>(handler));
    }



    template <typename Body, typename Allocator, typename Send>
    void PlayersList(http::request<Body, http::basic_fields<Allocator>>& req,
                         RequestInfo& req_info, Send&& send)
    {
        using namespace game_manager;
        PlayersResponse(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send),
            [&send, &req]
            (std::optional<std::vector<Player>> players, Result res)
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
            }
        );
    }

    template <typename Body, typename Allocator, typename Send>
    void PlayersState(http::request<Body, http::basic_fields<Allocator>>& req,
                         RequestInfo& req_info, Send&& send)
    {
        using namespace game_manager;
        PlayersResponse(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send),
            [&send, &req]
            (std::optional<std::vector<Player>> players, Result res)
            {
                using namespace resp_maker::json_resp;
                using namespace json_keys;
                if (res == Result::ok) {
                    json::object obj;

                    for (const Player& player : *players) {
                        const game_manager::State& state = player.state;
                        json::value jv = {
                            {"pos", json::array{state.coor.x, state.coor.y}},
                            {"speed", json::array{state.speed.x_axis, state.speed.y_axis}},
                            {"dir", game_manager::GetDirectionChar(state.dir)}
                        };
                        obj.emplace(std::to_string(player.id), jv);
                    }

                    json::object res;
                    res["players"s] = obj;

                    send(MakeOkResponse(req, json::serialize(res), true));
                } else if (res == Result::no_token) {
                    send(MakeUnauthorizedResponse(req, unknown_token_mess, unknown_token_key, true));
                } else if (res == Result::no_session) {
                    send(MakeNotFoundResponse(req, "Player`s session not found", "sessionNotFound", true));
                } else {
                    send(MakeUnknownErrorResponse(req, "Unknown error in Players lambda", "unknownError", true));
                }
            }
        );
    }

    ResponseInfo BadResponseInfo(std::string message = ""s, bool no_cache = false) {
        ResponseInfo res;
        res.body = resp_maker::json_resp::GetBadRequestResponseBody(std::move(message));
        res.no_cache = no_cache;
        res.status = http::status::bad_request;
        res.content_type = body_type::json;
        return res;
    }

    ResponseInfo NotFoundInfo(std::string message = ""s, bool no_cache = false) {
        ResponseInfo res;
        res.body = resp_maker::json_resp::GetNotFoundResponseBody(std::move(message));
        res.no_cache = no_cache;
        res.status = http::status::not_found;
        res.content_type = body_type::json;
        return res;
    }

    template <typename Body, typename Allocator>
    RequestInfo MakeRequestInfo (http::request<Body, http::basic_fields<Allocator>>& req) {
        RequestInfo result;
        result.target = req.target();
        result.body = req.body();
        result.method = req.method();
        http::request<http::string_body> r;

        if (req.find(http::field::content_type) != req.end()) {
            result.content_type = req.at(http::field::content_type);
        }

        return result;
    }

    bool FindAndCutTarget(RequestInfo& req, std::string_view part) {
        if (req.target.starts_with(part)) {
            req.target = req.target.substr(part.size());
            return true;
        }
        return false;
    }

    game_manager::GameManager& game_;
};

} // namespace api_handler
