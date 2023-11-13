#pragma once

// На данный момент неизвестно количество игроков в сессии
// поэтому используется максимум для int, который получаем из
// библиотеки limits. Когда количество игроков станет известно
// эту библиотеку можно больше не использовать
#include <limits>

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
#include "loot_generator.h"
#include "collision_detector.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast.hpp>

namespace game_manager {

class GameProvider : public collision_detector::ItemGathererProvider {
public:
    void AddItem(collision_detector::Item item) { items_.push_back(item); }
    size_t ItemsCount() const override { return items_.size(); }
    collision_detector::Item GetItem(size_t idx) const override { return items_.at(idx); }

    void AddGatherer(collision_detector::Gatherer gatherer) { gatherers_.push_back(gatherer); }
    size_t GatherersCount() const override { return gatherers_.size(); }
    collision_detector::Gatherer GetGatherer(size_t idx) const override { return gatherers_.at(idx); }
private:
    std::vector<collision_detector::Item> items_;
    std::vector<collision_detector::Gatherer> gatherers_;
};

namespace detail {
struct TokenTag {};
}  // namespace detail

namespace net = boost::asio;
using Token = util::Tagged<std::string, detail::TokenTag>;
using PlayerId = uint32_t;
namespace beast = boost::beast;
using TokenStr = std::string;
using namespace std::literals;

struct PlayerInfo {
    PlayerId Id;
    std::string token;
    std::string name;
};

struct ItemInfo {
    size_t id;
    size_t type;
};

struct Player {
    PlayerId id;
    std::string name;
    move_manager::State state;
    std::vector<ItemInfo> items_in_bag;
    size_t score = 0;
    void Move(uint64_t dur) {
        state.Move(dur);
    }
};

struct LootObject {
    size_t type;
    move_manager::PositionState position;
    size_t id;
    bool collected = false;
};

using LootObjectsContainer = std::deque<LootObject>;

struct PlayersAndObjects {
    const std::deque<Player>& players;
    const LootObjectsContainer& objects;
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
                 const move_manager::Map& move_map, model::LootConfig config,
                 bool random_spawn, uint64_t tick_duration = 0);

    bool BookPlace();

    template <class Handler>
    void AddPlayer(PlayerInfo info, Handler&& handler);

    template<class Handler>
    void GetPlayers(Handler&& handler) const;

    template<class Handler>
    void MovePlayer(PlayerId player_id, move_manager::Direction dir, Handler&& handler);

    void Tick(uint64_t duration);

    void GenerateLoot(uint64_t dur);
private:
    int GetRandomLootObject();

    void AddPlayersToProvider(GameProvider& provider, uint64_t duration);

    void AddItemsToProvider(GameProvider& provider);

    void AddOfficiesToProvider(GameProvider& provider);

    void HandleGatherEvents(std::vector<collision_detector::GatheringEvent>& events);

    void HandleCollisions(uint64_t duration);

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

    size_t object_id_ = 0;
    LootObjectsContainer loot_objects_;
    double loot_interval_;
    double loot_prob_;
    loot_gen::LootGenerator loot_generator_{static_cast<int>(loot_interval_ * 1000) * 1ms, loot_prob_/*,
                [this]()mutable{
                  std::uniform_real_distribution<>dis (0.0, 1.0);
                  return dis(generator);
                }
    */};
    // Закомментированный фрагмент меняет генератор случайных чисел
    // с дефолтного (тестового) на реальный
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

    std::deque<GameSession> sessions_;
    std::unordered_map<model::Map::Id, std::vector<GameSession*>, MapHasher> sessions_for_maps_;
    std::unordered_map<PlayerId, GameSession*> players_for_sessions_;

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

//===================================Templates implementation============================================

namespace game_manager {

template <class Handler>
void GameSession::AddPlayer(PlayerInfo info, Handler&& handler) {
    net::dispatch(strand_,
        [this, info = std::move(info), handler = std::forward<Handler>(handler)]
        {
            move_manager::State state;

            if (random_spawn_) {
                state.position = move_map_.GetRandomPlace();
            } else {
                state.position = move_map_.GetStartPlace();
            }

            players_.emplace_back(info.Id, std::move(info.name), state);
            id_for_player_[info.Id] = &players_.back();
            handler(info);
        }
    );
}

template<class Handler>
void GameSession::GetPlayers(Handler&& handler) const {
    net::dispatch(
        strand_,
        [this, handler = std::forward<Handler>(handler)](){
            PlayersAndObjects res{players_, loot_objects_};
            handler(res, Result::ok);
        }
    );
}

template<class Handler>
void GameSession::MovePlayer(PlayerId player_id, move_manager::Direction dir, Handler&& handler) {
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

template<class Handler>
void GameManager::Join(std::string name, model::Map::Id map, Handler&& handler) {
    PlayerInfo p_info;
    p_info.name = std::move(name);
    p_info.Id = GetUniquePlayerId();
    AddPlayer(std::move(p_info), std::move(map), std::forward<Handler>(handler));
}

template<class Handler>
void GameManager::AddPlayer(PlayerInfo p_info, model::Map::Id map, Handler&& handler) {
    net::dispatch(
        tokens_strand_,
        [this, p_info = std::move(p_info), map = std::move(map), handler = std::forward<Handler>(handler)]()mutable{
            Token token = GetUniqueToken();
            while (tokens_.find(token) != tokens_.end()) {
                token = GetUniqueToken();
            }
            p_info.token = *token;
            tokens_[token] = p_info.Id;
            FindOrCeateSession(std::move(p_info), std::move(map), std::forward<Handler>(handler));
        }
    );
}

template<class Handler>
void GameManager::FindOrCeateSession(PlayerInfo p_info, model::Map::Id map, Handler&& handler) {
    net::dispatch(
        sessions_strand_,
        [this, p_info = std::move(p_info), map = std::move(map), handler = std::forward<Handler>(handler)]()mutable{
            auto it = sessions_for_maps_.find(map);
            GameSession* session = nullptr;

            if (it == sessions_for_maps_.end()) {
                sessions_.emplace_back(ioc_, *FindMap(map), *maps_index_.at(map), game_.GetLootConfig(), random_spawn_);
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
                    sessions_.emplace_back(ioc_, *FindMap(map), *maps_index_.at(map), game_.GetLootConfig(), random_spawn_);
                    sessions_for_maps_[map].push_back(&sessions_.back());
                    session = &sessions_.back();
                }
            }

            players_for_sessions_[p_info.Id]   = session;
            session->AddPlayer(std::move(p_info), std::forward<Handler>(handler));
        }
    );
}

template<class Handler>
void GameManager::GetPlayers(Token token, Handler&& handler) {
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
void GameManager::MovePlayer(Token token, move_manager::Direction dir, Handler&& handler) {
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
void GameManager::CallTick(uint64_t duration, Handler&& handler) {
    if (test_mode_) {
        Tick(duration, std::forward<Handler>(handler));
    } else {
        throw std::logic_error("Game not in test mode");
    }
}

template<class Handler>
void GameManager::Tick(u_int64_t duration, Handler&& handler) {
    net::dispatch(
        sessions_strand_,
        [this, duration, handler = std::forward<Handler>(handler)] (){
            for (GameSession& session : sessions_) {
                session.Tick(duration);
            }
            handler(Result::ok);
        }
    );
}

template<class Handler>
void GameManager::FindToken(Token token, Handler&& handler) {
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
void GameManager::FindSession(Token token, Handler&& handler) {
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
} // namespace game_manager
