namespace api_handler {
template <typename Body, typename Allocator, typename Send>
void ApiHandler::operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

    std::string_view target = req.target();
    using ResponseType = http::response<Body, http::basic_fields<Allocator>>;
    ResponseType response;
    ResponseInfo response_info;
    RequestInfo req_info = MakeRequestInfo(req);

    if (FindAndCutTarget(req_info, http_strs::game_path)) {
        if (FindAndCutTarget(req_info, http_strs::join)) {
            return JoinResponse(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send));
        } else if (FindAndCutTarget(req_info, http_strs::players)) {
            return PlayersList(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send));
        } else if (FindAndCutTarget(req_info, http_strs::state)) {
            return PlayersState(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send));
        } else if (FindAndCutTarget(req_info, http_strs::player_action)) {
            return MoveResponse(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send));
        } else if (FindAndCutTarget(req_info, http_strs::tick)) {
            return TickResponse(std::forward<decltype(req)>(req), req_info, std::forward<Send>(send));
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


template <typename Body, typename Allocator, typename Send>
void ApiHandler::JoinResponse(http::request<Body, http::basic_fields<Allocator>>&& req, RequestInfo& req_info, Send&& send) {
    using namespace resp_maker;

    json::value player;

    if(!CheckRequest(req, req_info, send, true, Method::post, std::nullopt, true, &player)) {
        return;
    }

    if (!player.as_object().contains(json_keys::user_name_key) || !player.as_object().contains(json_keys::map_id_key)) {
        return send(json_resp::MakeBadRequestResponse(req, "Invalid join request format", "invalidArgument", true));
    }

    std::string user_name;
    std::string map_id;

    user_name = player.at(json_keys::user_name_key).as_string();
    map_id    = player.at(json_keys::map_id_key).as_string();

    if (user_name.find_first_not_of(' ') == user_name.npos) {
        return send(json_resp::MakeBadRequestResponse(req, "Invalid name", "invalidArgument", true));
    }

    if (game_.FindMap(model::Map::Id{map_id}) == nullptr) {
        return send(json_resp::MakeNotFoundResponse(req, "Map not found", true));
    }

    //Trim name???

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
        return send(json_resp::MakeBadRequestResponse(req, "Invalid endpoint", true));
    }

    json::value jv;

    if (!CheckRequest(req, req_info, send, true, Method::post, std::nullopt, true, &jv)) {
        return;
    }

    if (!jv.is_object() || !jv.as_object().contains(json_keys::time_delta_key)) {
        return send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
    }

    uint64_t duration;

    boost::json::string dur_boost_str;

    json::value dur = jv.at(json_keys::time_delta_key);

    if (dur.is_double() || dur.if_bool()) {
        return send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
    }

    if (dur.if_string()) {
        try {
            dur_boost_str = jv.at(json_keys::time_delta_key).as_string();
        } catch (...) {
            return send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
        }
        std::string dur_str{dur_boost_str.data(), dur_boost_str.size()};

        if (dur_str.find_first_not_of("0123456789") != dur_str.npos) {
            return send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
        }
        duration = std::stoi(dur_str);
    } else if (dur.is_int64()) {
        try {
            duration = dur.as_int64();
        } catch (...) {
            return send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
        }
    } else if (dur.is_uint64()) {
        try {
            duration = dur.as_uint64();
        } catch (...) {
            return send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
        }
    } else {
        return send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
    }

    if (duration == 0) {
        return send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
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

    game_manager::Token token{""};

    json::value jv;

    if (!CheckRequest(req, req_info, send, true, Method::post, &token, true, &jv)) {
        return;
    }


    if (!jv.is_object() || !jv.as_object().contains(json_keys::move_key)) {
        return send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
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
        return send(json_resp::MakeBadRequestResponse(req, "Failed to parse action", "invalidArgument", true));
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

    if (req_info.content_type != body_type::json) {
        send(MakeBadRequestResponse(req, "Unsupported format", true));
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
            **value_out = json::parse(std::string(req_info.body));
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
                    {json_keys::pos_key, json::array{state.coor.x, state.coor.y}},
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
ApiHandler::RequestInfo ApiHandler::MakeRequestInfo (http::request<Body, http::basic_fields<Allocator>>& req) {
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

} // namespace api_handler
