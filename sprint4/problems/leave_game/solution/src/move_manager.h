#pragma once

#include <deque>
#include <optional>
#include <unordered_map>
#include <random>
#include <cassert>

#include "model.h"

namespace move_manager {

class PointHasher {
public:
    size_t operator()(const model::Point& point) const;
};

struct Coords {
    double x;
    double y;
    bool operator==(const Coords&) const = default;
};

struct Speed {
    double x_axis = 0;
    double y_axis = 0;
    bool operator==(Speed other) const {
        return x_axis == other.x_axis && y_axis == other.y_axis;
    }
    bool IsNull() const {
        return x_axis == 0 && y_axis == 0;
    }
};

enum class Direction {
    NORTH,
    EAST,
    SOUTH,
    WEST,
    NONE
};

std::string GetStringDirection(Direction dir);

std::optional<Direction> GetDirectionFromString(std::string_view dir);

class Area;

struct MoveResult{
    const Area* area;
    Coords coords;
};

class Area{
public:
    Area(model::Point base) : base_(base) {}
    void SetUp(Area* area) { u_ = area; }
    void SetRight(Area* area) { r_ = area; }
    void SetDown(Area* area) { d_ = area; }
    void SetLeft(Area* area) { l_ = area; }

    model::Point GetBase() const { return base_; }
    const Area* GetNeighbour(Direction dir) const;
    bool IsIntersects(Coords coords, Direction dir, bool borders = true, bool other_axis = false) const;
    double GetMaxCoor(Direction dir) const;
    Coords Place(Coords coor) const;

private:
    model::Point base_;
    Area* u_ = nullptr;
    Area* r_ = nullptr;
    Area* d_ = nullptr;
    Area* l_ = nullptr;
};

struct PositionState {
    Coords coor;
    const Area* area;
};

struct State{
    Speed speed;
    Direction dir = Direction::NORTH;
    PositionState position;
    void Move(uint64_t dur);
};

class Map {
public:
    using AreaMap = std::unordered_map<model::Point, Area*, PointHasher>;

    Map(const model::Map& map) : Map(map.GetRoads()) {}

    Map(const std::vector<model::Road>& roads);

    PositionState GetStartPlace() const;

    PositionState GetRandomPlace() const;

    void PlaceCoors(std::vector<PositionState*>& positions) const {
        std::unordered_map<model::Point, const Area*, PointHasher> point_to_area;

        for (const Area& area : areas_) {
            point_to_area[area.GetBase()] = &area;
        }

        for (PositionState* pos : positions) {
            model::Point point;
            point.x = static_cast<int>(pos->coor.x + 0.5);
            point.y = static_cast<int>(pos->coor.y + 0.5);
            pos->area = point_to_area.at(point);
        }
    }
private:
    void AddRoad (const model::Road& road, AreaMap& area_map);

    void AddArea (model::Point point, AreaMap& area_map);

    void AddNeigbours(Area& area, AreaMap& area_map);

    const Area& GetRandomArea() const;

    Coords GetRandomOffset() const;

    std::deque<Area> areas_;
};
} // namespace move_manager
