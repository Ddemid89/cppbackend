#include "game_manager.h"
#include "collision_detector.h"

#include <iostream>

namespace game_manager {

using namespace std::literals;

GameSession::GameSession (net::io_context& ioc, const model::Map& map,
             const move_manager::Map& move_map, model::LootConfig config, bool random_spawn, uint64_t tick_duration)
    : strand_(net::make_strand(ioc)),
      map_(map),
      move_map_(move_map),
      random_spawn_(random_spawn),
      loot_interval_(config.period),
      loot_prob_(config.probability)
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

void GameSession::Tick(uint64_t duration) {
    net::dispatch(
        strand_,
        [this, duration](){
            HandleCollisions(duration);
            for (Player& player : players_) {
                player.Move(duration);
            }
            GenerateLoot(duration);
        }
    );
}

int GameSession::GetRandomLootObject() {
    static std::uniform_int_distribution<size_t> dis(0, map_.GetLootTypes().size() - 1);
    return dis(generator);
}

void GameSession::HandleCollisions(uint64_t duration) {
    using namespace collision_detector;
    GameProvider provider;

    for (const Player& player : players_) {
        const move_manager::State& state = player.state;
        geom::Point2D start{state.position.coor.x, state.position.coor.y};
        double dx = (state.speed.x_axis * duration) / 1000;
        double dy = (state.speed.y_axis * duration) / 1000;
        geom::Point2D finish {start.x + dx, start.y + dy};
        provider.AddGatherer({start, finish, model::DOG_WIDTH / 2});
    }

    for (const LootObject& loot_object : loot_objects_) {
        const move_manager::PositionState& state = loot_object.position;
        geom::Point2D pos{state.coor.x, state.coor.y};
        provider.AddItem({pos, model::ITEM_WIDTH / 2, false});
    }

    for (const model::Office& office : map_.GetOffices()) {
        double x = office.GetPosition().x;
        double y = office.GetPosition().y;
        geom::Point2D pos{x, y};
        provider.AddItem({pos, model::OFFICE_WIDTH / 2, true});
    }

    auto gather_events = FindGatherEvents(provider);

    for (const auto& event : gather_events) {
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

void GameSession::GenerateLoot(uint64_t dur) {
    using namespace std::literals;
    int loot_to_generate = loot_generator_.Generate(dur * 1ms, loot_objects_.size(), players_.size());
    while (loot_to_generate-- > 0) {
        loot_objects_.emplace_back(GetRandomLootObject(), move_map_.GetRandomPlace(), object_id_++);
    }
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
} // namespace game_manager

