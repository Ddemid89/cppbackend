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

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast.hpp>


#include <iostream>


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

struct Coords {
    double x;
    double y;
};

struct Speed {
    double x_axis;
    double y_axis;
};

enum class Direction {
    NORTH,
    EAST,
    SOUTH,
    WEST
};

std::string GetDirectionChar(Direction dir) {
    using namespace std::literals;
    if (dir == Direction::NORTH) {
        return "U"s;
    } else if (dir == Direction::SOUTH) {
        return "D"s;
    } else if (dir == Direction::EAST) {
        return "R"s;
    } else if (dir == Direction::EAST) {
        return "L"s;
    } else {
        throw std::logic_error("not supported direction");
    }
}

struct State{
    Coords coor;
    Speed speed = {0, 0};
    Direction dir = Direction::NORTH;
};

struct Player {
    PlayerId id;
    std::string name;
    State state;
};

enum class Result {
    no_token,
    no_session,
    ok
};

class GameSession{
public:
    const int max_players = std::numeric_limits<int>::max();
    GameSession (net::io_context& ioc, const model::Map& map)
        : strand_(net::make_strand(ioc)),
          map_(map) {
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
                State state;
                state.coor = RandomPlace();
                players_.emplace_back(info.Id, info.name, state);
                handler(info);
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

private:
    const model::Road& GetRandomRoad(){
        static std::uniform_int_distribution<int> dis{0, static_cast<int>(map_.GetRoads().size() - 1)};
        return map_.GetRoads().at(dis(generator));
    }

    Coords RandomPlace() {
        const model::Road& road = GetRandomRoad();
        Coords res;

        double single_coor, pair_coor_start, pair_coor_finish;

        if (road.IsHorizontal()) {
            single_coor = road.GetStart().y;
            pair_coor_start = std::min(road.GetStart().x, road.GetEnd().x);
            pair_coor_finish = std::max(road.GetStart().x, road.GetEnd().x);
        } else {
            single_coor = road.GetStart().x;
            pair_coor_start = std::min(road.GetStart().y, road.GetEnd().y);
            pair_coor_finish = std::max(road.GetStart().y, road.GetEnd().y);
        }

        std::uniform_real_distribution<> dis(pair_coor_start, pair_coor_finish);

        if (road.IsHorizontal()) {
            res.y = single_coor;
            res.x = dis(generator);
        } else {
            res.x = single_coor;
            res.y = dis(generator);
        }

        return res;
    }

    const model::Map& map_;
    net::strand<net::io_context::executor_type> strand_;
    std::vector<Player> players_;
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
              sessions_strand_(net::make_strand(ioc_)) {
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
        PlayerInfo p_info;
        p_info.name = std::move(name);
        p_info.Id = GetUniquePlayerId();
        AddPlayer(std::move(p_info), std::move(map), std::forward<Handler>(handler));
    }
private:
    template<class Handler>
    void AddPlayer(PlayerInfo p_info, model::Map::Id map, Handler&& handler) {
        net::dispatch(
            tokens_strand_,
            [this, p_info = std::move(p_info), map = std::move(map), handler = std::forward<Handler>(handler)]()mutable{
                p_info.token = GetUniqueToken();
                while (tokens_.find(p_info.token) != tokens_.end()) {
                    p_info.token = GetUniqueToken();
                }
                tokens_[p_info.token] = p_info.Id;
                FindSession(std::move(p_info), std::move(map), std::forward<Handler>(handler));
            }
        );
    }

    template<class Handler>
    void FindSession(PlayerInfo p_info, model::Map::Id map, Handler&& handler) {
        net::dispatch(
            sessions_strand_,
            [this, p_info = std::move(p_info), map = std::move(map), handler = std::forward<Handler>(handler)]()mutable{
                auto it = sessions_for_maps_.find(*map);
                GameSession* session = nullptr;

                if (it == sessions_for_maps_.end()) {
                    sessions_.emplace_back(ioc_, *FindMap(map));
                    sessions_for_maps_[*map].push_back(&sessions_.back());
                    session = &sessions_.back();
                } else {
                    for (GameSession* sess : it->second) {
                        if (sess->BookPlace()) {
                            session = sess;
                            break;
                        }
                    }
                    if (session == nullptr) {
                        sessions_.emplace_back(ioc_, *FindMap(map));
                        sessions_for_maps_[*map].push_back(&sessions_.back());
                        session = &sessions_.back();
                    }
                }

                players_for_sessions_[p_info.Id]   = session;
                session->AddPlayer(std::move(p_info), std::forward<Handler>(handler));
            }
        );
    }

    PlayerId GetUniquePlayerId() {
        return player_counter_.fetch_add(1);
    }

    std::string GetUniqueToken() {
        std::stringstream stream;
        stream.fill('0');
        stream << std::hex << std::setw(16) << generator1_();
        stream << std::hex << std::setw(16) << generator2_();
        return stream.str();
    } 
public:
    template<class Handler>
    void GetPlayers(Token token, Handler&& handler) {
        net::dispatch(
            tokens_strand_,
            [this, token = std::forward<Token>(token), handler = std::forward<Handler>(handler)]()mutable{
                auto it = tokens_.find(*token);

                if (it != tokens_.end()) {
                    PlayerId id = it->second;
                    GetSession(id, std::forward<Handler>(handler));
                } else {
                    handler(std::nullopt, Result::no_token);
                }
            }
        );
    }
private:
    template<class Handler>
    void GetSession(PlayerId id, Handler&& handler) {
        net::dispatch(
            sessions_strand_,
            [this, id, handler = std::forward<Handler>(handler)]()mutable{
                auto it = players_for_sessions_.find(id);
                if (it != players_for_sessions_.end()) {
                    it->second->GetPlayers(std::forward<Handler>(handler));
                } else {
                    handler(std::nullopt, Result::no_session);
                }
            }
        );
    }

    model::Game& game_;
    net::io_context& ioc_;


    // Выполнять действия с tokens_ только из tokens_strand_!!!!!!!!
    net::strand<net::io_context::executor_type> tokens_strand_;
    std::unordered_map<TokenStr, PlayerId> tokens_;

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
    std::unordered_map<TokenStr, std::vector<GameSession*>> sessions_for_maps_;
    std::unordered_map<PlayerId, GameSession*> players_for_sessions_;

    //std::unordered_map<TokenStr, GameSession*> tokens_for_sessions_; //Может сразу искать сессию по токену?

    /* Также нужно продумать удаление сессий и игроков (токенов)
     * из этих индексов
     */

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
