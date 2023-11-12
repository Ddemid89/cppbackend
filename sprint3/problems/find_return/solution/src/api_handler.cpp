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

void tag_invoke(json::value_from_tag, json::value& jv, const ItemInfo& info) {
    jv = {
        {id_key, info.id},
        {type_key, info.type}
    };
}

} // game_manager

namespace api_handler {

namespace beast = boost::beast;
namespace http = beast::http;
using namespace std::literals;
namespace json = boost::json;


} // namespace api_handler

