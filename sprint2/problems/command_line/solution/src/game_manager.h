#pragma once

// Когда будет извесно точное количество игроков
// в сессии, можно удалить
#include <limits> // <- это

#include <deque>
#include <unordered_map>
#include <optional>
#include <vector>
#include <random>
#include <iomanip>
#include <sstream>

#include "model.h"
#include "move_manager.h"
#include "tagged.h"
#include "ticker.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast.hpp>


namespace game_manager {

namespace detail {
struct TokenTag {};
}  // namespace detail

namespace net = boost::asio;
using Token = util::Tagged<std::string, detail::TokenTag>;
using PlayerId = uint32_t;
namespace beast = boost::beast;
using TokenStr = std::string;

struct PlayerInfo {
    PlayerId Id;
    std::string token;
    std::string name;
};

struct Player {
    PlayerId id;
    std::string name;
    move_manager::State state;
    void Move(uint64_t dur) {
        state.Move(dur);
    }
};

enum class Result {
    no_token,
    no_session,
    ok
};

class GameSession{
public:
    const int max_players = std::numeric_limits<int>::max();
    GameSession (net::io_context& ioc, const model::Map& map,
                 const move_manager::Map& move_map, bool random_spawn, uint64_t tick_duration = 0);

    bool BookPlace();

    template <class Handler>
    void AddPlayer(PlayerInfo info, Handler&& handler);

    template<class Handler>
    void GetPlayers(Handler&& handler) const;

    template<class Handler>
    void MovePlayer(PlayerId player_id, move_manager::Direction dir, Handler&& handler);

    void Tick(uint64_t duration);
private:
    move_manager::Speed GetSpeed(move_manager::Direction dir);

    const model::Map& map_;
    const move_manager::Map& move_map_;
    net::strand<net::io_context::executor_type> strand_;
    std::unordered_map<PlayerId, Player*> id_for_player_;
    std::deque<Player> players_;
    //Сразу бронируем место для создателя сессии
    std::atomic<size_t> players_number_ = 1;

    std::random_device rd;
    std::mt19937 generator = std::mt19937(rd());
    bool random_spawn_;
};

//-----------------------------------GameManager------------------------------------------

class GameManager {
public:
    using Maps = std::deque<model::Map>;
    const int TOKEN_SIZE = 32;

    GameManager(model::Game& game, net::io_context& ioc, bool random_spawn, uint64_t tick_duration);

    model::Game& GetGame() { return game_; }
    const Maps GetMaps() const noexcept { return game_.GetMaps(); }

    const std::vector<model::MapInfo>& GetMapsInfo() const noexcept {
        return game_.GetMapsInfo();
    }

    const model::Map* FindMap(const model::Map::Id& id) const noexcept {
        return game_.FindMap(id);
    }

    template<class Handler>
    void Join(std::string name, model::Map::Id map, Handler&& handler);
private:
    template<class Handler>
    void AddPlayer(PlayerInfo p_info, model::Map::Id map, Handler&& handler);

    template<class Handler>
    void FindOrCeateSession(PlayerInfo p_info, model::Map::Id map, Handler&& handler);

    PlayerId GetUniquePlayerId() {
        return player_counter_.fetch_add(1);
    }

    Token GetUniqueToken();
public:
    template<class Handler>
    void GetPlayers(Token token, Handler&& handler);

    template<class Handler>
    void MovePlayer(Token token, move_manager::Direction dir, Handler&& handler);

    template<class Handler>
    void CallTick(uint64_t duration, Handler&& handler);

    bool IsTestMode() const {
        return test_mode_;
    }

private:
    template<class Handler>
    void Tick(u_int64_t duration, Handler&& handler);


    template<class Handler>
    void FindToken(Token token, Handler&& handler);

    template<class Handler>
    void FindSession(Token token, Handler&& handler);

    model::Game& game_;
    net::io_context& ioc_;

    using MapHasher = util::TaggedHasher<model::Map::Id>;
    using TokenHasher = util::TaggedHasher<Token>;

    // Выполнять действия с tokens_ только из tokens_strand_!!!!!!!!
    net::strand<net::io_context::executor_type> tokens_strand_;
    std::unordered_map<Token, PlayerId, TokenHasher> tokens_;

    std::atomic<PlayerId> player_counter_ = 0;


    // Выполнять все действия с sessions, sessions_for_maps_ и
    // players_for_sessions_ только из session_strand_!!!!!!!!!!!!!!!!
    net::strand<net::io_context::executor_type> sessions_strand_;


    /* Нужно подумать над контейнерами
     * Хотелось бы быстро находить сессию со свободными местами,
     * и, видимо, придется удалять сессии, которые уже закончились
     * При этом хотелось бы, чтоб указатели не инвалидировались
     */
    std::deque<GameSession> sessions_;
    std::unordered_map<model::Map::Id, std::vector<GameSession*>, MapHasher> sessions_for_maps_;
    std::unordered_map<PlayerId, GameSession*> players_for_sessions_;

    //std::unordered_map<TokenStr, GameSession*> tokens_for_sessions_; //Может сразу искать сессию по токену?
    /* Также нужно продумать удаление сессий и игроков (токенов)
     * из этих индексов
     */

    std::unordered_map<model::Map::Id, move_manager::Map*, MapHasher> maps_index_;
    std::vector<move_manager::Map> maps_;


    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

    bool random_spawn_;
    uint64_t tick_duration_;
    bool test_mode_;
};
} // namespace game_manager

#include "game_manager.tpp"
