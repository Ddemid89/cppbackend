#pragma once

// На данный момент неизвестно количество игроков в сессии
// поэтому используется максимум для int, который получаем из
// библиотеки limits. Когда количество игроков станет известно
// эту библиотеку можно больше не использовать
#include <limits>

#include <deque>
#include <unordered_map>
#include <unordered_set>
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

class TickListner {
public:
    virtual void Notify(uint64_t duration) = 0;
};

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
    bool operator==(const ItemInfo&) const = default;
};

struct Player {
    PlayerId id;
    std::string name;
    move_manager::State state;
    std::vector<ItemInfo> items_in_bag;
    size_t score = 0;
    size_t idle_time = 0;
    size_t game_time = 0;
    void Move(uint64_t dur) {
        state.Move(dur);
    }
};

struct LootObject {
    size_t type;
    move_manager::Coords position;
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

struct GameSessionRepr {
    std::string map_name;
    std::vector<Player> players;
    std::vector<LootObject> loot_objects;
};

struct PlayerRepr {
    std::string token;
    size_t id;
};

struct Retiree {
    Retiree() = default;
    explicit Retiree(const Player& player) : name(player.name),
                                             score(player.score),
                                             game_time(player.game_time),
                                             id(player.id) {}
    std::string name;
    size_t score;
    size_t game_time;
    size_t id;
};


class RecordSaverInterface {
public:
    virtual void Save(std::vector<Retiree>&&) = 0;
    virtual std::vector<Retiree> GetRecords(size_t start, size_t max_size) = 0;
};

struct GameRepr {
    size_t players_number;
    std::vector<GameSessionRepr> sessions;
    std::vector<PlayerRepr> players;
};

class GameSession;

template <class Callback>
class GameReprWrapper {
public:
    GameReprWrapper (Callback&& callback)
            : callback_(std::forward<Callback>(callback)){}

    void AddSession(size_t index, GameSessionRepr&& session) {
        repr_.sessions.at(index) = std::move(session);
        filled_sessions_++;
        if (filled_sessions_ == sessions_number_) {
            callback_(std::move(repr_));
        }
    }

    void SetSessionsNumber(size_t number) {
        if (number == 0) {
            callback_(std::move(repr_));
        }
        sessions_number_ = number;
        repr_.sessions.resize(number);
    }

    void AddPlayers(std::vector<PlayerRepr>&& players, size_t number) {
        repr_.players_number = number;
        repr_.players = std::move(players);
    }

private:
    std::atomic<size_t> filled_sessions_ = 0;
    std::atomic<size_t> sessions_number_;

    GameRepr repr_;
    Callback callback_;
};

class GameSession{
public:
    const int max_players = std::numeric_limits<int>::max();
    GameSession (net::io_context& ioc, const model::Map& map,
                 const move_manager::Map& move_map, model::LootConfig config,
                 bool random_spawn, double retirement_time, uint64_t tick_duration = 0);

    bool BookPlace();

    template <class Handler>
    void AddPlayer(PlayerInfo info, Handler&& handler);

    template<class Handler>
    void GetPlayers(Handler&& handler) const;

    template<class Handler>
    void MovePlayer(PlayerId player_id, move_manager::Direction dir, Handler&& handler);

    template<class Callback>
    void Tick(uint64_t duration, Callback&& remove_retirees);

    GameSessionRepr GetRepresentation();

    template <class Callback>
    void GetRepresentationAsync(Callback&& callback);

    void Restore(GameSessionRepr&& repr);
private:
    std::vector<Retiree> GetAndRemoveRetires(size_t duration);

    void MovePlayers(size_t duration);

    void GenerateLoot(uint64_t dur);

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

    size_t retirement_time_ms_;

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

    GameRepr GetRepresentation();

    template <class Callback>
    void GetRepresentationAsync(Callback&& callback);

    void Restore(GameRepr&& repr);

    void Subscribe(std::shared_ptr<TickListner> listner) {
        listners_.push_back(std::move(listner));
    }
    void SetRecordSaver(const std::shared_ptr<RecordSaverInterface>& newRecord_saver);

    std::vector<Retiree> GetRecords(size_t start, size_t max_items) const;

private:
    template<class ReprType>
    void AddPlayersForRepr(ReprType repr);

    template<class ReprType>
    void AddSessionsForRepr(ReprType repr);

    template<class Handler>
    void Tick(u_int64_t duration, Handler&& handler);

    template<class Handler>
    void FindToken(Token token, Handler&& handler);

    template<class Handler>
    void FindSession(Token token, Handler&& handler);

    void SaveRecords(std::vector<Retiree>&& retirees);

    void DeleteOnePlayer(PlayerId id);

    void DeletePlayersFromSession(std::vector<Retiree>&& retirees);

    void DeletePlayers(std::vector<Retiree>&& retirees);

    model::Game& game_;
    net::io_context& ioc_;

    using MapHasher = util::TaggedHasher<model::Map::Id>;
    using TokenHasher = util::TaggedHasher<Token>;

    std::shared_ptr<RecordSaverInterface> record_saver_;

    // Выполнять действия с tokens_ только из tokens_strand_!!!!!!!!
    net::strand<net::io_context::executor_type> tokens_strand_;
    std::unordered_map<Token, PlayerId, TokenHasher> tokens_;
    std::unordered_map<PlayerId, Token> id_to_tokens_;

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

    std::vector<std::shared_ptr<TickListner>> listners_;
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

            std::string name;

            if (info.name == "") {
                name = "no name";
            } else {
                name = std::move(info.name);
            }

            players_.emplace_back(info.Id, std::move(name), state);
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
                    player.idle_time = 0;
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
            tokens_.emplace(token, p_info.Id);
            id_to_tokens_.emplace(p_info.Id, token);
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
                sessions_.emplace_back(ioc_, *FindMap(map), *maps_index_.at(map),
                                       game_.GetLootConfig(), random_spawn_, game_.GetRetirementTime());
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
                    sessions_.emplace_back(ioc_, *FindMap(map), *maps_index_.at(map),
                                           game_.GetLootConfig(), random_spawn_, game_.GetRetirementTime());
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
                session.Tick(duration,
                    [this](std::vector<Retiree>&& retirees){
                        if (!retirees.empty()) {
                            DeletePlayers(std::move(retirees));
                        }
                    }
                );
            }
            for (std::shared_ptr<TickListner>& listner : listners_) {
                listner->Notify(duration);
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

template<class Callback>
void GameSession::Tick(uint64_t duration, Callback&& remove_retirees) {
    net::dispatch(
        strand_,
        [this, duration, remove_retirees = std::forward<Callback>(remove_retirees)](){
            auto retirees = GetAndRemoveRetires(duration);

            HandleCollisions(duration);
            MovePlayers(duration);

            remove_retirees(std::move(retirees));

            GenerateLoot(duration);
        }
    );
}

template<class ReprType>
void GameManager::AddSessionsForRepr(ReprType repr) {
    net::dispatch(sessions_strand_,
        [repr, this](){
            int i = 0;
            repr->SetSessionsNumber(sessions_.size());

            for (GameSession& session : sessions_) {
                session.GetRepresentationAsync(
                    [repr, i](GameSessionRepr&& sess){
                        repr->AddSession(i, std::move(sess));
                    }
                );

                i++;
            }
        }
    );
}

template <class Callback>
void GameSession::GetRepresentationAsync(Callback&& callback) {
    net::dispatch(strand_,
        [callback = std::forward<Callback>(callback), this](){
            GameSessionRepr result;

            result.map_name = *map_.GetId();
            result.players = std::vector<Player>{players_.begin(), players_.end()};
            result.loot_objects = std::vector<LootObject>{loot_objects_.begin(), loot_objects_.end()};

            callback(std::move(result));
        }
    );
}

template<class ReprType>
void GameManager::AddPlayersForRepr(ReprType repr) {
    net::dispatch(tokens_strand_,
        [repr, this](){
            std::vector<PlayerRepr> players(tokens_.size());

            for (const auto& [token, id] : tokens_) {
                players.emplace_back(*token, id);
            }

            repr->AddPlayers(std::move(players), player_counter_);

            AddSessionsForRepr(repr);
        }
    );
}

template <class Callback>
void GameManager::GetRepresentationAsync(Callback&& callback) {
    using ReprType = GameReprWrapper<Callback>;

    std::shared_ptr<ReprType> repr = std::make_shared<ReprType>(
                std::forward<Callback>(callback)
    );

    AddPlayersForRepr(repr);
}

} // namespace game_manager
