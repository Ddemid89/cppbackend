#include "api_handler.h"

namespace game_manager {
namespace json = boost::json;
using namespace json_keys;

void tag_invoke (json::value_from_tag, json::value& jv, const PlayerInfo& pi) {
    jv = {
        {auth_token_key, pi.token},
        {player_id_key, pi.Id},
    };
}

void tag_invoke(json::value_from_tag, json::value& jv, const ItemInfo& info) {
    jv = {
        {id_key, info.id},
        {type_key, info.type}
    };
}

void tag_invoke(json::value_from_tag, json::value& jv, const Retiree& ret) {
    jv = {
        {name_key, ret.name},
        {score_key, ret.score},
        {playtime_key, static_cast<double>(ret.game_time) / 1000}
    };
}

} // game_manager

namespace api_handler {

namespace beast = boost::beast;
namespace http = beast::http;
using namespace std::literals;
namespace json = boost::json;

ApiHandler::ApiHandler(game_manager::GameManager& game) : game_(game) {}

void ApiHandler::HandleGameResponse() {
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
    } else if (path_part == http_strs::records) {
        HandleRecordsRequest();
    } else {
        SendBadRequestResponseDefault();
    }
}

bool ApiHandler::CheckEndPath() {
    if (req_info_.target == "/"sv || req_info_.target.empty()) {
        return true;
    }
    return false;
}

bool ApiHandler::CheckMethod(Method method) {
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

bool ApiHandler::CheckAuth() {
    if (req_info_.auth.has_value()) {
        if ((**req_info_.auth).size() == game_.TOKEN_SIZE) {
            return true;
        }
    }
    return false;
}

bool ApiHandler::CheckRequest(Method method, bool auth) {
    if (!CheckEndPath()) {
        SendBadRequestResponse("Unsupported request"s);
        return false;
    }

    if (auth && !CheckAuth()) {
        SendNoAuthResponse();
        return false;
    }

    if (!CheckMethod(method)) {
        SendWrongMethodResponse(method);
        return false;
    }

    return true;
}

void ApiHandler::HandleJoinRequest() {
    using namespace resp_maker;

    if(!CheckRequest(Method::post, false)) {
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

void ApiHandler::HandlePlayersStateRequest() {
    using namespace resp_maker::json_resp;

    if(!CheckRequest(Method::get_head, true)) {
        return;
    }

    using namespace game_manager;
    game_.GetPlayers(*req_info_.auth,
                     [self = this->shared_from_this()]
                     (const std::optional<PlayersAndObjects>& players_and_objects, Result res) {
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

void ApiHandler::HandlePlayersListRequest() {
    using namespace resp_maker::json_resp;

    game_manager::Token token{""};

    if(!CheckRequest(Method::get_head, true)) {
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
                json::object val;
                val.emplace(json_keys::name_key, player.name);
                obj.emplace(std::to_string(player.id), val);
            }
            self->SendOkResponse(json::serialize(obj));
        } else if (res == Result::no_token) {
            self->SendNoAuthResponse(json_keys::unknown_token_mess, json_keys::unknown_token_key);
        } else {
            self->SendNotFoundResponse("Player`s session not found", "sessionNotFound");
        }
    });
}

std::optional<uint32_t> ApiHandler::TryGetNumberFromJson(json::value& jv, const std::string& key) {
    if (!jv.is_object() || !jv.as_object().contains(key)) {
        SendBadRequestResponse("Failed to parse action", "invalidArgument");
        return std::nullopt;
    }

    uint64_t result;

    json::value res = jv.at(key);

    if (res.is_double() || res.is_bool()) {
        SendBadRequestResponse("Failed to parse action", "invalidArgument");
        return std::nullopt;
    }

    if (res.is_string()) {
        boost::json::string res_boost_str;

        try {
            res_boost_str = res.as_string();
        } catch (...) {
            SendBadRequestResponse("Failed to parse action", "invalidArgument");
            return std::nullopt;
        }
        std::string res_str{res_boost_str.data(), res_boost_str.size()};

        if (res_str.find_first_not_of("0123456789") != res_str.npos || res_str.empty()) {
            SendBadRequestResponse("Failed to parse action", "invalidArgument");
            return std::nullopt;
        }
        result = std::stoi(res_str);
    } else if (res.is_int64()) {
        try {
            result = res.as_int64();
        } catch (...) {
            SendBadRequestResponse("Failed to parse action", "invalidArgument");
            return std::nullopt;
        }
    } else if (res.is_uint64()) {
        try {
            result = res.as_uint64();
        } catch (...) {
            SendBadRequestResponse("Failed to parse action", "invalidArgument");
            return std::nullopt;
        }
    } else {
        SendBadRequestResponse("Failed to parse action", "invalidArgument");
        return std::nullopt;
    }

    return result;
}

void ApiHandler::HandleTickRequest() {
    using namespace resp_maker;

    if (!game_.IsTestMode()) {
        SendBadRequestResponse("Invalid endpoint");
        return;
    }

    if(!CheckRequest(Method::post, false)) {
        return;
    }

    json::value jv;

    try {
        jv = json::parse(req_info_.body);
    } catch (...) {
        SendBadRequestResponse("Failed to parse action", "invalidArgument");
        return;
    }

    auto duration = TryGetNumberFromJson(jv, json_keys::time_delta_key);

    if (!duration.has_value()) {
        return;
    }

    if (*duration == 0) {
        SendBadRequestResponse("Failed to parse action", "invalidArgument");
        return;
    }

    game_.CallTick(*duration,
                   [self = this->shared_from_this()](game_manager::Result res){
        self->SendOkResponse("{}");
    }
    );
}

void ApiHandler::HandleMoveRequest() {
    using namespace resp_maker;

    std::string path_part = FindAndCutTarget(req_info_);
    if (path_part != http_strs::action) {
        SendBadRequestResponseDefault();
        return;
    }

    if(!CheckRequest(Method::post, true)) {
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

void ApiHandler::HandleRecordsRequest() {
    if (req_info_.max_number > MAX_ITEMS) {
        SendBadRequestResponse("MaxItems max value is 100");
        return;
    }

    if(!CheckRequest(Method::get_head, false)) {
        return;
    }

    auto records = game_.GetRecords(req_info_.start, req_info_.max_number);

    json::value result = json::value_from(records);

    SendOkResponse(json::serialize(result));
}

json::value ApiHandler::MakePlayersJson(const std::deque<game_manager::Player>& players) {
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

json::value ApiHandler::MakeLootObjectsJson(const game_manager::LootObjectsContainer& objects) {
    using namespace game_manager;
    json::object obj;
    for (const LootObject& object : objects) {
        move_manager::Coords coords = object.position;
        json::value jv = {
            {json_keys::type_key, object.type},
            {json_keys::pos_key, json::array{coords.x, coords.y}}
        };
        obj.emplace(std::to_string(object.id), jv);
    }
    return obj;
}

void ApiHandler::HandleApiResponse() {
    std::string path_part = FindAndCutTarget(req_info_);
    if (path_part == http_strs::v1) {
        HandleV1Response();
        return;
    } else {
        SendBadRequestResponseDefault();
        return;
    }
}

void ApiHandler::HandleV1Response() {
    std::string path_part = FindAndCutTarget(req_info_);
    if (path_part == http_strs::maps) {
        HandleMapsResponse();
        return;
    } else {
        SendBadRequestResponseDefault();
        return;
    }
}

void ApiHandler::HandleMapsResponse() {
    if (CheckEndPath()) {
        json::value jv = json::value_from(game_.GetMapsInfo());
        SendOkResponse(json::serialize(jv));
        return;
    } else {
        HandleOneMapResponse();
        return;
    }
}

void ApiHandler::HandleOneMapResponse() {
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

std::string ApiHandler::FindAndCutTarget(RequestInfo& req) {
    std::string res;

    size_t q_pos = req.target.find_last_of('?');

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

ApiHandler::ResponseInfo ApiHandler::MakeResponse(http::status status, bool no_cache) {
    ResponseInfo result;

    result.status = status;
    result.version = req_info_.version;
    result.content_type = body_type::json;
    result.keep_alive = req_info_.keep_alive;
    result.no_cache = no_cache;

    return result;
}

void ApiHandler::SendOkResponse(const std::string& body, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::ok, no_cache);

    result.body = body;

    send_(result);
}

void ApiHandler::SendBadRequestResponse(std::string message, std::string code, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::bad_request, no_cache);

    json::value body = {
        {json_keys::code_key, code},
        {json_keys::message_key, message}
    };

    result.body = json::serialize(body);

    send_(result);
}

void ApiHandler::SendNotFoundResponse(const std::string& message, const std::string& key, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::not_found, no_cache);

    json::value body = {
        {json_keys::code_key, key},
        {json_keys::message_key, message}
    };

    result.body = json::serialize(body);

    send_(result);
}

void ApiHandler::SendNoAuthResponse(const std::string& message, const std::string& key, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::unauthorized, no_cache);

    json::value body = {
        {json_keys::code_key, key},
        {json_keys::message_key, message}
    };

    result.body = json::serialize(body);

    send_(result);
}

void ApiHandler::SendWrongMethodResponseAllowedGetHead(const std::string& message, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::method_not_allowed, no_cache);

    json::value body = {
        {json_keys::code_key, json_keys::invalid_method_key},
        {json_keys::message_key, message}
    };

    result.body = json::serialize(body);

    result.additional_fields.emplace_back(http::field::allow, "GET, HEAD"s);

    send_(result);
}

void ApiHandler::SendWrongMethodResponseAllowedPost(const std::string& message, bool no_cache) {
    ResponseInfo result = MakeResponse(http::status::method_not_allowed, no_cache);

    json::value body = {
        {json_keys::code_key, json_keys::invalid_method_key},
        {json_keys::message_key, message}
    };

    result.body = json::serialize(body);

    result.additional_fields.emplace_back(http::field::allow, "POST"s);

    send_(result);
}

void ApiHandler::SendWrongMethodResponse(Method method) {
    switch(method) {
    case Method::get_head:
        SendWrongMethodResponseAllowedGetHead();
        break;
    case Method::post:
        SendWrongMethodResponseAllowedPost();
        break;
    case Method::any:
        throw std::logic_error("Any can`t be wrong method");
    }
}

void ApiHandler::SetStartOrMaxNumber(RequestInfo& info, const std::string& key, int val) {
    if (key == json_keys::start_key) {
        info.start = val;
    } else if (key == json_keys::max_items_key) {
        info.max_number = val;
    }
}

void ApiHandler::ParseStartAndMaxNumber(RequestInfo& info) {
    size_t qs_pos = info.target.find_first_of('?');
    if (qs_pos != info.target.npos) {
        size_t amp_pos = info.target.find_first_of('&');
        if (amp_pos != info.target.npos) {
            size_t eq_pos = info.target.find_first_of('=', qs_pos);
            std::string key1 = info.target.substr(qs_pos + 1, eq_pos - qs_pos - 1);
            std::string val1 = info.target.substr(eq_pos + 1, amp_pos - eq_pos - 1);

            eq_pos = info.target.find_first_of('=', amp_pos);
            std::string key2 = info.target.substr(amp_pos + 1, eq_pos - amp_pos - 1);
            std::string val2 = info.target.substr(eq_pos + 1, info.target.npos);

            int val1int = std::stoi(val1);
            int val2int = std::stoi(val2);

            SetStartOrMaxNumber(info, key1, val1int);
            SetStartOrMaxNumber(info, key2, val2int);

        } else {
            size_t eq_pos = info.target.find_first_of('=', qs_pos);
            std::string key = info.target.substr(qs_pos + 1, eq_pos - qs_pos - 1);
            std::string val = info.target.substr(eq_pos + 1, info.target.npos);
            int valint = std::stoi(val);

            SetStartOrMaxNumber(info, key, valint);
        }
        info.target = info.target.substr(0, qs_pos);
    }
}

} // namespace api_handler

