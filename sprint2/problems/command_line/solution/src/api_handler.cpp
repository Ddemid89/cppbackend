#include "api_handler.h"

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



ApiHandler::ResponseInfo ApiHandler::ApiResponse(RequestInfo &req) {
    if (FindAndCutTarget(req, "/v1"sv)) {
        return V1Response(req);
    } else {
        return BadResponseInfo();
    }
}

ApiHandler::ResponseInfo ApiHandler::V1Response(RequestInfo& req) {
    if (FindAndCutTarget(req, "/maps"sv)) {
        return MapsResponse(req);
    } else {
        return BadResponseInfo();
    }
}

ApiHandler::ResponseInfo ApiHandler::MapsResponse(RequestInfo& req) {
    if (req.target == "/"sv || req.target.empty()) {
        json::value jv = json::value_from(game_.GetMapsInfo());
        return {http::status::ok, json::serialize(jv), body_type::json, true};
    } else {
        return OneMapResponse(req);
    }
}

ApiHandler::ResponseInfo ApiHandler::OneMapResponse(RequestInfo& req) {
    if (req.target.find_last_of('/') != 0) {
        return BadResponseInfo();
    } else {
        model::Map::Id map_id{std::string(req.target.substr(1))};
        auto map_ptr = game_.FindMap(map_id);

        if (map_ptr == nullptr) {
            return NotFoundInfo("Map not found", true);
        } else {
            json::value jv = json::value_from(*map_ptr);
            return {http::status::ok, json::serialize(jv), body_type::json, true};
        }
    }
}

ApiHandler::ResponseInfo ApiHandler::BadResponseInfo(std::string message, bool no_cache) {
    ResponseInfo res;
    res.body = resp_maker::json_resp::GetBadRequestResponseBody(std::move(message));
    res.no_cache = no_cache;
    res.status = http::status::bad_request;
    res.content_type = body_type::json;
    return res;
}

ApiHandler::ResponseInfo ApiHandler::NotFoundInfo(std::string message, bool no_cache) {
    ResponseInfo res;
    res.body = resp_maker::json_resp::GetNotFoundResponseBody(std::move(message));
    res.no_cache = no_cache;
    res.status = http::status::not_found;
    res.content_type = body_type::json;
    return res;
}

bool ApiHandler::FindAndCutTarget(RequestInfo& req, std::string_view part) {
    if (req.target.starts_with(part)) {
        req.target = req.target.substr(part.size());
        return true;
    }
    return false;
}
} // namespace api_handler
