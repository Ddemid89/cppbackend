#include "game_manager.h"

// Когда будет извесно точное количество игроков
// в сессии, можно удалить
//#include <limits> // <- это

//#include <deque>
//#include <unordered_map>
//#include <optional>
//#include <vector>
//#include <random>
//#include <iomanip>
//#include <sstream>

//#include "model.h"
//#include "move_manager.h"
//#include "tagged.h"
//#include "ticker.h"

//#include <boost/asio/io_context.hpp>
//#include <boost/asio/strand.hpp>
//#include <boost/beast.hpp>


namespace game_manager {

//namespace net = boost::asio;
//using Token = util::Tagged<std::string, detail::TokenTag>;
//using PlayerId = uint32_t;
//namespace beast = boost::beast;
//using TokenStr = std::string;

GameSession::GameSession (net::io_context& ioc, const model::Map& map,
             const move_manager::Map& move_map, bool random_spawn, uint64_t tick_duration)
    : strand_(net::make_strand(ioc)),
      map_(map),
      move_map_(move_map),
      random_spawn_(random_spawn)
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
            for (Player& player : players_) {
                player.Move(duration);
            }
        }
    );
}

move_manager::Speed GameSession::GetSpeed(move_manager::Direction dir) {
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

