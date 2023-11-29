#include "game_serialization.h"

namespace move_manager {

void serialize(boost::archive::text_oarchive& ar, PositionState& pos, const unsigned version) {
    ar& pos.coor;
}

void serialize(boost::archive::text_iarchive& ar, PositionState& pos, const unsigned version) {
    ar& pos.coor;
    pos.area = nullptr;
}

} // namespace move_manager

namespace game_serialization {

void Serializator::Notify(uint64_t duration) {
    static bool has_period = HasPeriod();
    last_save_ += duration;
    if (last_save_ > period_) {
        last_save_ = 0;
        SaveAsync();
    }
}

void Serializator::SaveAsync() {
    game_.GetRepresentationAsync([this](game_manager::GameRepr&& repr) {
        SaveRepr(std::move(repr));
    });
}

void Serializator::Save() {
    SaveRepr(game_.GetRepresentation());
}

void Serializator::SaveRepr(game_manager::GameRepr&& repr) {
    using namespace std::literals;

    std::filesystem::path file(file_);
    std::filesystem::path tmp = file.parent_path() / "tmp";

    std::ofstream out(tmp);
    boost::archive::text_oarchive archive(out);
    archive << repr;

    std::filesystem::rename(tmp, file);
}

void Serializator::Load() {
    if (!std::filesystem::exists(file_)) {
        return;
    }
    game_manager::GameRepr repr;
    std::ifstream in(file_);
    boost::archive::text_iarchive archive(in);
    archive >> repr;
    game_.Restore(std::move(repr));
}

bool Serializator::HasPeriod() {
    if (period_ == 0) {
        throw std::logic_error("Notified seializator does not have a period set");
    }
    return true;
}

} // namespace game_serialization
