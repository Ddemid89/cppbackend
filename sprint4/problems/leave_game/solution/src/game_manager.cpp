#include "game_manager.h"
#include "collision_detector.h"

#include <iostream>

namespace game_manager {

using namespace std::literals;

GameSession::GameSession (net::io_context& ioc, const model::Map& map,
             const move_manager::Map& move_map, model::LootConfig config,
             bool random_spawn, double retirement_time_s, uint64_t tick_duration)
    : strand_(net::make_strand(ioc)),
      map_(map),
      move_map_(move_map),
      random_spawn_(random_spawn),
      loot_interval_(config.period),
      loot_prob_(config.probability),
      retirement_time_ms_(retirement_time_s * 1000)
{
}

bool GameSession::BookPlace() {
    size_t new_size = players_number_.fetch_add(1);

    if (new_size > max_players) {
        players_number_.fetch_sub(1);
        return false;
    }
    return true;
}

void GameSession::MovePlayers(size_t duration) {
    for (Player& player : players_) {
        player.Move(duration);
    }
}

std::vector<Retiree> GameSession::GetAndRemoveRetires(size_t duration) {
    std::vector<Retiree> res;

    auto it = std::copy_if(players_.begin(), players_.end(), players_.begin(),
                           [this, &res, duration] (Player& player) {
        player.game_time += duration;

        if (!player.state.speed.IsNull()) {
            player.idle_time = 0;
            return true;
        }

        player.idle_time += duration;
        if (player.idle_time >= retirement_time_ms_) {
            res.emplace_back(player);
            id_for_player_.erase(player.id);
            players_number_--;
            return false;
        }
        return true;
    }
    );

    players_.erase(it, players_.end());

    for (Player& player : players_) {
        id_for_player_[player.id] = &player;
    }

    return res;
}

int GameSession::GetRandomLootObject() {
    static std::uniform_int_distribution<size_t> dis(0, map_.GetLootTypes().size() - 1);
    return dis(generator);
}

void GameSession::AddPlayersToProvider(GameProvider& provider, uint64_t duration) {
    for (const Player& player : players_) {
        const move_manager::State& state = player.state;
        geom::Point2D start{state.position.coor.x, state.position.coor.y};
        double dx = (state.speed.x_axis * duration) / 1000;
        double dy = (state.speed.y_axis * duration) / 1000;
        geom::Point2D finish {start.x + dx, start.y + dy};
        provider.AddGatherer({start, finish, model::DOG_WIDTH / 2});
    }
}

void GameSession::AddItemsToProvider(GameProvider& provider) {
    for (const LootObject& loot_object : loot_objects_) {
        const move_manager::Coords& coor = loot_object.position;
        geom::Point2D pos{coor.x, coor.y};
        provider.AddItem({pos, model::ITEM_WIDTH / 2, false});
    }
}

void GameSession::AddOfficiesToProvider(GameProvider& provider) {
    for (const model::Office& office : map_.GetOffices()) {
        double x = office.GetPosition().x;
        double y = office.GetPosition().y;
        geom::Point2D pos{x, y};
        provider.AddItem({pos, model::OFFICE_WIDTH / 2, true});
    }
}

void GameSession::HandleGatherEvents(std::vector<collision_detector::GatheringEvent>& events) {
    for (const auto& event : events) {
        Player& player = players_.at(event.gatherer_id);

        if (player.items_in_bag.size() < map_.GetBagCapacity()) {
            if (event.is_office) {
                for (ItemInfo item : player.items_in_bag) {
                    player.score += map_.GetLootTypes().at(item.type).value;
                }
                player.items_in_bag.clear();
            } else {
                LootObject& item = loot_objects_.at(event.item_id);
                if (!item.collected && player.items_in_bag.size() < map_.GetBagCapacity()) {
                    player.items_in_bag.push_back({item.id, item.type});
                    item.collected = true;
                }
            }
        }
    }
    int new_size = 0;
    size_t pos = 0;
    for(int i = 0; i < loot_objects_.size(); ++i) {
        if (!loot_objects_.at(i).collected) {
            loot_objects_.at(pos) = loot_objects_.at(i);
            new_size++;
            pos++;
        }
    }
    loot_objects_.resize(new_size);
}

void GameSession::HandleCollisions(uint64_t duration) {
    using namespace collision_detector;
    GameProvider provider;

    AddPlayersToProvider(provider, duration);
    AddItemsToProvider(provider);
    AddOfficiesToProvider(provider);

    auto gather_events = FindGatherEvents(provider);

    HandleGatherEvents(gather_events);
}

void GameSession::GenerateLoot(uint64_t dur) {
    using namespace std::literals;
    int loot_to_generate = loot_generator_.Generate(dur * 1ms, loot_objects_.size(), players_.size());
    while (loot_to_generate-- > 0) {
        loot_objects_.emplace_back(GetRandomLootObject(), move_map_.GetRandomPlace().coor, object_id_++);
    }
}

GameSessionRepr GameSession::GetRepresentation() {
    GameSessionRepr result;
    result.map_name = *map_.GetId();
    result.players = std::vector<Player>{players_.begin(), players_.end()};
    result.loot_objects = std::vector<LootObject>{loot_objects_.begin(), loot_objects_.end()};

    return result;
}

void GameSession::Restore(GameSessionRepr&& repr) {
    loot_objects_ = LootObjectsContainer{
            std::make_move_iterator(repr.loot_objects.begin()),
            std::make_move_iterator(repr.loot_objects.end())
};
    object_id_ = loot_objects_.size();

    players_ = std::deque<Player>{
            std::make_move_iterator(repr.players.begin()),
            std::make_move_iterator(repr.players.end())
};
    players_number_ = players_.size();

    std::vector<move_manager::PositionState*> positions;

    positions.reserve(players_.size());

    for (Player& player : players_) {
        id_for_player_[player.id] = &player;
        positions.push_back(&player.state.position);
    }

    move_map_.PlaceCoors(positions);
}

move_manager::Speed GameSession::GetSpeed(move_manager::Direction dir) {
    using namespace move_manager;
    switch (dir) {
    case Direction::NORTH:
        return {0., -map_.GetDogSpeed()};
    case Direction::SOUTH:
        return {0., map_.GetDogSpeed()};
    case Direction::EAST:
        return {map_.GetDogSpeed(), 0.};
    case Direction::WEST:
        return {-map_.GetDogSpeed(), 0.};
    case Direction::NONE:
        return {0., 0.};
    }
    throw std::logic_error("GameSession::GetSpeed: not implemented");
}

GameManager::GameManager(model::Game& game, net::io_context& ioc, bool random_spawn, uint64_t tick_duration)
    : game_(game),
      ioc_(ioc),
      tokens_strand_(net::make_strand(ioc_)),
      sessions_strand_(net::make_strand(ioc_)),
      random_spawn_(random_spawn),
      tick_duration_(tick_duration),
      test_mode_(tick_duration_ == 0)
{
    auto maps = game_.GetMaps();
    maps_.reserve(maps.size());
    for (const model::Map& map : maps) {
        maps_.emplace_back(map);
        maps_index_[map.GetId()] = &maps_.back();
    }
    if (!test_mode_) {
        auto ticker = std::make_shared<ticker::Ticker>(sessions_strand_, std::chrono::milliseconds{tick_duration_},
            [this](std::chrono::milliseconds delta) {
                Tick(std::chrono::duration_cast<std::chrono::milliseconds>(delta).count(), [](Result){});
            }
        );
        ticker->Start();
    }
}

Token GameManager::GetUniqueToken() {
    std::stringstream stream;
    stream.fill('0');
    stream << std::hex << std::setw(16) << generator1_();
    stream << std::hex << std::setw(16) << generator2_();
    return Token{stream.str()};
}

GameRepr GameManager::GetRepresentation() {
    GameRepr result;

    result.players_number = player_counter_;

    std::unordered_map<GameSession*, size_t> sessions_ptr_to_index_;

    for (GameSession& session : sessions_) {
        result.sessions.push_back(session.GetRepresentation());
        sessions_ptr_to_index_[&session] = sessions_ptr_to_index_.size();
    }

    for (const auto& [token, id] : tokens_) {
        GameSession* sess_ptr = players_for_sessions_.at(id);
        size_t session_index = sessions_ptr_to_index_.at(sess_ptr);
        result.players.emplace_back(*token, id);
    }

    return result;
}

void GameManager::Restore(GameRepr&& repr) {
    player_counter_ = repr.players_number;

    std::unordered_set<PlayerId> existing_players;

    for (PlayerRepr& player : repr.players) {
        Token token{player.token};
        tokens_[token] = player.id;
        id_to_tokens_.emplace(player.id, std::move(token));
        existing_players.insert(player.id);
    }

    for (GameSessionRepr& sess : repr.sessions) {
        model::Map::Id map_id{sess.map_name};
        const model::Map& map = *game_.FindMap(map_id);
        const move_manager::Map& move_map = *maps_index_.at(map_id);

        sessions_.emplace_back(ioc_, map, move_map, game_.GetLootConfig(),
                               random_spawn_, game_.GetRetirementTime());

        std::vector<Player> valid_players;
        valid_players.reserve(sess.players.size());

        for (Player& player : sess.players) {
            size_t id = player.id;
            if (existing_players.contains(id)) {
                valid_players.push_back(std::move(player));
                players_for_sessions_[id] = &sessions_.back();
            }
        }

        std::swap(valid_players, sess.players);

        sessions_.back().Restore(std::move(sess));
        sessions_for_maps_[map_id].push_back(&sessions_.back());
    }

}

void GameManager::SetRecordSaver(const std::shared_ptr<RecordSaverInterface>& newRecord_saver) {
    record_saver_ = newRecord_saver;
}

std::vector<Retiree> GameManager::GetRecords(size_t start, size_t max_items) const {
    if (!record_saver_) {
        throw std::logic_error("No record saver");
    }

    return record_saver_->GetRecords(start, max_items);
}

void GameManager::SaveRecords(std::vector<Retiree>&& retirees) {
    if (record_saver_) {
        record_saver_->Save(std::move(retirees));
    }
}

void GameManager::DeleteOnePlayer(PlayerId id) {
    Token token{id_to_tokens_.at(id)};
    tokens_.erase(token);
    id_to_tokens_.erase(id);
}

void GameManager::DeletePlayersFromSession(std::vector<Retiree>&& retirees) {
    net::dispatch(
                sessions_strand_,
                [this, retirees = std::move(retirees)]() mutable {
        for (const Retiree& ret : retirees) {
            players_for_sessions_.erase(ret.id);
        }

        SaveRecords(std::move(retirees));
    }
    );
}

void GameManager::DeletePlayers(std::vector<Retiree>&& retirees) {
    net::dispatch(
                tokens_strand_,
                [this, retirees = std::move(retirees)] () mutable {
        for (const Retiree& ret : retirees) {
            DeleteOnePlayer(ret.id);
        }

        DeletePlayersFromSession(std::move(retirees));
    }
    );
}
} // namespace game_manager

