#include <fstream>
#include <filesystem>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>

#include "../src/move_manager.h"
#include "../src/game_manager.h"

namespace move_manager {

template <typename Archive>
void serialize(Archive& ar, Coords& coords, [[maybe_unused]] const unsigned version) {
    ar& coords.x;
    ar& coords.y;
}

template <typename Archive>
void serialize(Archive& ar, Speed& speed, [[maybe_unused]] const unsigned version) {
    ar& speed.x_axis;
    ar& speed.y_axis;
}

void serialize(boost::archive::text_oarchive& ar, PositionState& pos,  [[maybe_unused]] const unsigned version) {
    ar& pos.coor;
}

void serialize(boost::archive::text_iarchive& ar, PositionState& pos,  [[maybe_unused]] const unsigned version) {
    ar& pos.coor;
    pos.area = nullptr;
}

template <typename Archive>
void serialize(Archive& ar, State& state, [[maybe_unused]] const unsigned version) {
    ar& state.dir;
    ar& state.position;
    ar& state.speed;
}

} // namespace move_manager

namespace game_manager {

template <typename Archive>
void serialize(Archive& ar, ItemInfo& info, [[maybe_unused]] const unsigned version) {
    ar& info.id;
    ar& info.type;
}

template <typename Archive>
void serialize(Archive& ar, LootObject& item, [[maybe_unused]] const unsigned version) {
    ar& item.position;
    ar& item.id;
    ar& item.type;
    ar& item.collected;
}

template <typename Archive>
void serialize(Archive& ar, Player& player, [[maybe_unused]] const unsigned version) {
    ar& player.id;
    ar& player.items_in_bag;
    ar& player.name;
    ar& player.score;
    ar& player.state;
}

template <typename Archive>
void serialize(Archive& ar, PlayerRepr& player, [[maybe_unused]] const unsigned version) {
    ar& player.id;
    ar& player.token;
}

template <typename Archive>
void serialize(Archive& ar, GameSessionRepr& session, [[maybe_unused]] const unsigned version) {
    ar& session.loot_objects;
    ar& session.map_name;
    ar& session.players;
}

template <typename Archive>
void serialize(Archive& ar, GameRepr& game, [[maybe_unused]] const unsigned version) {
    ar& game.players;
    ar& game.players_number;
    ar& game.sessions;
}

} // namespace game_manager

namespace game_serialization {
class Serializator : public game_manager::TickListner {
public:
    Serializator(game_manager::GameManager& game, std::string file, uint64_t period)
        : game_(game), file_(std::move(file)), period_(period) {}

    void Notify(uint64_t duration) override {
        static bool has_period = HasPeriod();
        last_save_ += duration;
        if (last_save_ > period_) {
            last_save_ = 0;
            SaveAsync();
        }
    }

    void SaveAsync() {
        game_.GetRepresentationAsync([this](game_manager::GameRepr&& repr) {
            SaveRepr(std::move(repr));
        });
    }

    void Save() {
        SaveRepr(game_.GetRepresentation());
    }

    void SaveRepr(game_manager::GameRepr&& repr) {
        std::filesystem::path file(file_);
        std::filesystem::path tmp = file.parent_path() / "tmp";

        std::ofstream out(tmp);
        boost::archive::text_oarchive archive(out);
        archive << repr;

        std::filesystem::rename(tmp, file);
    }

    void Load() {
        if (!std::filesystem::exists(file_)) {
            return;
        }
        game_manager::GameRepr repr;
        std::ifstream in(file_);
        boost::archive::text_iarchive archive(in);
        archive >> repr;
        game_.Restore(std::move(repr));
    }
private:
    bool HasPeriod() {
        if (period_ == 0) {
            throw std::logic_error("Notified seializator does not have a period set");
        }
        return true;
    }

    game_manager::GameManager& game_;
    std::string file_;
    uint64_t period_ = 0;
    uint64_t last_save_ = 0;
};
} // namespace game_serialization
