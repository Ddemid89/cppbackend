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

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast.hpp>




#include <iostream>
#include "my_debug.h"




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
        state.area->Move(state, dur);
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
    GameSession (net::io_context& ioc, const model::Map& map, const move_manager::Map& move_map)
        : strand_(net::make_strand(ioc)),
          map_(map),
          move_map_(move_map)
    {
    }

    bool BookPlace() {
        size_t new_size = players_number_.fetch_add(1);

        if (new_size > max_players) {
            players_number_.fetch_sub(1);
            return false;
        }
        return true;
    }

    template <class Handler>
    void AddPlayer(PlayerInfo info, Handler&& handler) {
        net::dispatch(strand_,
            [this, info = std::move(info), handler = std::forward<Handler>(handler)]
            {
                LOG("Session::AddPlayer");
                move_manager::State state;
                //move_manager::MoveResult position = move_map_.GetRandomPlace();
                move_manager::MoveResult position = move_map_.GetStartPlace();
                state.coor = position.coords;
                state.area = position.area;
                players_.emplace_back(info.Id, std::move(info.name), state);
                id_for_player_[info.Id] = &players_.back();
                LOG("Session::AddPlayer before handler call");
                handler(info);
                LOG("Session::AddPlayer after handler call");
            }
        );
    }

    template<class Handler>
    void GetPlayers(Handler&& handler) const {
        net::dispatch(
            strand_,
            [this, handler = std::forward<Handler>(handler)](){
                handler(players_, Result::ok);
            }
        );
    }

    template<class Handler>
    void MovePlayer(PlayerId player_id, move_manager::Direction dir, Handler&& handler) {
        net::dispatch(
            strand_,
            [this, player_id, dir, handler = std::forward<Handler>(handler)] {
                auto it = id_for_player_.find(player_id);

                if (it != id_for_player_.end()) {
                    Player& player = *it->second;
                    if (dir != move_manager::Direction::NONE) {
                        player.state.dir = dir;
                    }
                    player.state.speed = GetSpeed(dir);
                    handler(Result::ok);
                } else {
                    throw std::runtime_error("GameSession: player not found");
                }
            }
        );
    }

    void Tick(uint64_t duration) {
        net::dispatch(
            strand_,
            [this, duration](){
                for (Player& player : players_) {
                    player.Move(duration);
                }
            }
        );
    }
private:
    move_manager::Speed GetSpeed(move_manager::Direction dir) {
        if (dir == move_manager::Direction::NORTH) {
            return {0., -map_.GetDogSpeed()};
        } else if (dir == move_manager::Direction::SOUTH) {
            return {0., map_.GetDogSpeed()};
        } else if (dir == move_manager::Direction::EAST) {
            return {map_.GetDogSpeed(), 0.};
        } else if (dir == move_manager::Direction::WEST) {
            return {-map_.GetDogSpeed(), 0.};
        } else {
            return {0., 0.};
        }
    }

    const model::Map& map_;
    const move_manager::Map& move_map_;
    net::strand<net::io_context::executor_type> strand_;
    std::unordered_map<PlayerId, Player*> id_for_player_;
    std::deque<Player> players_;
    //Сразу бронируем место для создателя сессии
    std::atomic<size_t> players_number_ = 1;

    std::random_device rd;
    std::mt19937 generator = std::mt19937(rd());
};

//-----------------------------------GameManager------------------------------------------

class GameManager {
public:
    using Maps = std::deque<model::Map>;

    GameManager(model::Game& game, net::io_context& ioc)
            : game_(game),
              ioc_(ioc),
              tokens_strand_(net::make_strand(ioc_)),
              sessions_strand_(net::make_strand(ioc_))
    {
        auto maps = game_.GetMaps();
        maps_.reserve(maps.size());
        for (const model::Map& map : maps) {
            maps_.emplace_back(map);
            maps_index_[map.GetId()] = &maps_.back();
        }
    }

    model::Game& GetGame() { return game_; }
    const Maps GetMaps() const noexcept { return game_.GetMaps(); }

    const std::vector<model::MapInfo>& GetMapsInfo() const noexcept {
        return game_.GetMapsInfo();
    }

    const model::Map* FindMap(const model::Map::Id& id) const noexcept {
        return game_.FindMap(id);
    }

    template<class Handler>
    void Join(std::string name, model::Map::Id map, Handler&& handler) {
        LOG("GameManager::Join");
        PlayerInfo p_info;
        p_info.name = std::move(name);
        p_info.Id = GetUniquePlayerId();
        LOG("Before call AddPlayers");
        AddPlayer(std::move(p_info), std::move(map), std::forward<Handler>(handler));
    }
private:
    template<class Handler>
    void AddPlayer(PlayerInfo p_info, model::Map::Id map, Handler&& handler) {
        net::dispatch(
            tokens_strand_,
            [this, p_info = std::move(p_info), map = std::move(map), handler = std::forward<Handler>(handler)]()mutable{
                LOG("Lambda in AddPlayer");
                Token token = GetUniqueToken();
                while (tokens_.find(token) != tokens_.end()) {
                    token = GetUniqueToken();
                }
                p_info.token = *token;
                tokens_[token] = p_info.Id;
                LOG("Before FindOrCreateSession");
                FindOrCeateSession(std::move(p_info), std::move(map), std::forward<Handler>(handler));
            }
        );
    }

    template<class Handler>
    void FindOrCeateSession(PlayerInfo p_info, model::Map::Id map, Handler&& handler) {
        net::dispatch(
            sessions_strand_,
            [this, p_info = std::move(p_info), map = std::move(map), handler = std::forward<Handler>(handler)]()mutable{
                LOG("In FindOrCreate");
                auto it = sessions_for_maps_.find(map);
                GameSession* session = nullptr;

                if (it == sessions_for_maps_.end()) {
                    sessions_.emplace_back(ioc_, *FindMap(map), *maps_index_.at(map));
                    sessions_for_maps_[map].push_back(&sessions_.back());
                    session = &sessions_.back();
                } else {
                    for (GameSession* sess : it->second) {
                        if (sess->BookPlace()) {
                            session = sess;
                            break;
                        }
                    }
                    if (session == nullptr) {
                        sessions_.emplace_back(ioc_, *FindMap(map), *maps_index_.at(map));
                        sessions_for_maps_[map].push_back(&sessions_.back());
                        session = &sessions_.back();
                    }
                }

                LOG("Before session::AddPlayer");
                players_for_sessions_[p_info.Id]   = session;
                session->AddPlayer(std::move(p_info), std::forward<Handler>(handler));
            }
        );
    }

    PlayerId GetUniquePlayerId() {
        return player_counter_.fetch_add(1);
    }

    Token GetUniqueToken() {
        std::stringstream stream;
        stream.fill('0');
        stream << std::hex << std::setw(16) << generator1_();
        stream << std::hex << std::setw(16) << generator2_();
        return Token{stream.str()};
    } 
public:
    template<class Handler>
    void GetPlayers(Token token, Handler&& handler) {
        FindSession(token,
            [handler = std::forward<Handler>(handler)]
            (std::optional<GameSession*> session, PlayerId id, Result res)mutable{
                if (res == Result::ok) {
                    GameSession& sess = **session;
                    sess.GetPlayers(std::forward<Handler>(handler));
                } else {
                    handler(std::nullopt, res);
                }
            }
        );
    }

    template<class Handler>
    void MovePlayer(Token token, move_manager::Direction dir, Handler&& handler) {
        FindSession(std::move(token),
            [dir, handler = std::forward<Handler>(handler)]
            (std::optional<GameSession*> session, PlayerId id, Result res)mutable{
                if (res == Result::ok) {
                    GameSession& sess = **session;
                    sess.MovePlayer(id, dir, handler);
                } else {
                    handler(res);
                }
            }
        );
    }

    template<class Handler>
    void Tick(uint64_t duration, Handler&& handler) {
        net::dispatch(
            sessions_strand_,
            [this, duration, handler = std::forward<Handler>(handler)](){
                for (GameSession& session : sessions_) {
                    session.Tick(duration);
                }
                handler(Result::ok);
            }
        );
    }

private:
    template<class Handler>
    void FindToken(Token token, Handler&& handler) {
        net::dispatch(
            tokens_strand_,
            [this, token = std::move(token), handler = std::forward<Handler>(handler)]()mutable{
                auto it = tokens_.find(token);
                if (it != tokens_.end()) {
                    handler(it->second);
                } else {
                    handler(std::nullopt);
                }
            }
        );
    }

    template<class Handler>
    void FindSession(Token token, Handler&& handler) {
        FindToken(std::move(token),
            [this, handler = std::forward<Handler>(handler)](std::optional<PlayerId> id)mutable{
                if (id.has_value()) {
                    net::dispatch(sessions_strand_,
                        [this, id, handler = std::forward<Handler>(handler)]()mutable{
                            auto it = players_for_sessions_.find(*id);
                            if (it != players_for_sessions_.end()) {
                                handler(it->second, *id, Result::ok);
                            } else {
                                handler(std::nullopt, 0, Result::no_session);
                            }
                        }
                    );
                } else {
                    handler(std::nullopt, 0, Result::no_token);
                }
            }
        );
    }

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
};
} // namespace game_manager
