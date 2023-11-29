#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <optional>

#include "tagged.h"

namespace model {

const double DEFAULT_DOG_SPEED = 1;
const size_t DEFAULT_BAG_CAPACITY = 3;
const double DEFAULT_RETIREMENT_TIME = 60.;
const double DOG_WIDTH = 0.6;
const double ITEM_WIDTH = 0;
const double OFFICE_WIDTH = 0.5;
const double ROAD_WIDTH = 0.8;

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
    bool operator==(Point other) const {
        return x == other.x && y == other.y;
    }
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct LootConfig {
    double period;
    double probability;
};

enum class LootSort {
    obj
};

struct LootType {
    std::string name;
    std::string file;
    LootSort type;
    std::optional<int> rotation;
    std::string color;
    double scale;
    int value;
};

struct GameConfig {
    LootConfig loot_config;
    double default_speed = DEFAULT_DOG_SPEED;
    size_t default_capacity = DEFAULT_BAG_CAPACITY;
    double retirement_time = DEFAULT_RETIREMENT_TIME;
};

struct MapConfig{
    double dog_speed = 0;
    size_t bag_capacity = 0;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;
    using LootTypes = std::vector<LootType>;

    Map(Id id, std::string name, MapConfig config) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , dog_speed_(config.dog_speed)
        , defaultSpeed(config.dog_speed == 0)
        , bag_capacity_(config.bag_capacity)
    {}

    const Id& GetId() const noexcept {
        return id_;
    }

    std::string_view GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    const LootTypes& GetLootTypes() const noexcept {
        return loot_types_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddLootType(LootType object) {
        loot_types_.emplace_back(std::move(object));
    }

    void AddOffice(Office office);

    double GetDogSpeed() const {
        return dog_speed_;
    }

    void TrySetBagCapacity(size_t capacity) {
        if (bag_capacity_ == 0) {
            bag_capacity_ = capacity;
        }
    }

    size_t GetBagCapacity() const {
        if (bag_capacity_ != 0) {
            return bag_capacity_;
        }
        throw std::logic_error("Bag capacity is not defined");
    }

    bool IsDefaultSpeed() const {
        return defaultSpeed;
    }

    void SetDogSpeed(double value) {
        if (!defaultSpeed) {
            throw std::logic_error("Not default speed");
        }
        dog_speed_ = value;
    }
private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    LootTypes loot_types_;

    double dog_speed_;
    bool defaultSpeed;
    size_t bag_capacity_;
};

class MapInfo {
public:
    MapInfo(const Map::Id& id, std::string_view name) : id_(*id), name_(name) {}

    std::string_view GetId() const {
        return id_;
    }

    std::string_view GetName() const {
        return name_;
    }
private:
    std::string_view id_;
    std::string_view name_;
};

class Game {
public:
    using Maps = std::deque<Map>;

    Game(GameConfig config) : default_speed_(config.default_speed),
                                             loot_config_(config.loot_config),
                                             default_capacity_(config.default_capacity),
                                             retirement_time_(config.retirement_time){}

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const std::vector<MapInfo>& GetMapsInfo() const noexcept {
        return maps_info_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    LootConfig GetLootConfig() const {
        return loot_config_;
    }

    double GetRetirementTime() const {
        return retirement_time_;
    }


private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    Maps maps_;
    MapIdToIndex map_id_to_index_;
    std::vector<MapInfo> maps_info_;
    double default_speed_;
    size_t default_capacity_;
    LootConfig loot_config_;
    double retirement_time_;
};

}  // namespace model
