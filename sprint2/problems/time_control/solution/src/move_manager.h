#pragma once

#include <deque>
#include <optional>
#include <unordered_map>
#include <random>
#include <cassert>

#include "model.h"

//#include <iostream>


namespace move_manager {

class PointHasher {
public:
    size_t operator()(const model::Point& point) const {
        std::hash<int> hasher;
        size_t x_hash = hasher(point.x);
        size_t y_hash = hasher(point.y);
        return x_hash ^ (y_hash << 1);
    }
};

struct Coords {
    double x;
    double y;
};

struct Speed {
    double x_axis = 0;
    double y_axis = 0;
    bool operator==(Speed other) const {
        return x_axis == other.x_axis && y_axis == other.y_axis;
    }
};

enum class Direction {
    NORTH,
    EAST,
    SOUTH,
    WEST,
    NONE
};

std::string GetStringDirection(Direction dir) {
    using namespace std::literals;
    if (dir == Direction::NORTH) {
        return "U"s;
    } else if (dir == Direction::SOUTH) {
        return "D"s;
    } else if (dir == Direction::EAST) {
        return "R"s;
    } else if (dir == Direction::WEST) {
        return "L"s;
    } else {
        throw std::logic_error("Direction not supported");
    }
}

std::optional<Direction> GetDirectionFromString(std::string_view dir) {
    using namespace std::literals;
    if (dir == "U"sv) {
        return Direction::NORTH;
    } else if (dir == "D"sv) {
        return Direction::SOUTH;
    } else if (dir == "L"sv) {
        return Direction::WEST;
    } else if (dir == "R"sv) {
        return Direction::EAST;
    } else if (dir == ""sv) {
        return Direction::NONE;
    } else {
        return std::nullopt;
    }
}

class Area;

struct State{
    Coords coor;
    Speed speed;
    Direction dir = Direction::NORTH;
    const Area* area;
};

struct MoveResult{
    const Area* area;
    Coords coords;
};

//std::ostream& operator<<(std::ostream& out, Coords coords) {
//    //out << "{" << coords.x << ", " << coords.y << "}";
//    return out;
//}

//std::ostream& operator<<(std::ostream& out, model::Point coords) {
//    //out << "{" << coords.x << ", " << coords.y << "}";
//    return out;
//}

class Area{
public:
    Area(model::Point base) : base_(base) {}
    void SetUp(Area* area) {u_ = area;}
    void SetRight(Area* area)  {r_ = area;}
    void SetDown(Area* area) {d_ = area;}
    void SetLeft(Area* area)  {l_ = area;}
    model::Point GetBase() const {return base_;}

    void Move(State& state, uint64_t dur) const {
        if (state.speed == Speed{0., 0.}) {
            return;
        }

        //std::cerr << "Before: " << state.coor << std::endl;

        state.coor.x += (state.speed.x_axis * dur) / 1000;
        state.coor.y += (state.speed.y_axis * dur) / 1000;

        //std::cerr << "After: " << state.coor << std::endl;

        //std::cerr << "Area before: " << state.area->GetBase();

        if (state.dir == Direction::NORTH) {
            double offset = state.coor.y - base_.y;
            Move(state.coor.y, base_.y, offset, u_, false, state.area, state.dir, state.speed);
            //std::cerr << "Area after: " << state.area->GetBase();
            //std::cerr << "After: " << state.coor << std::endl;
        } else if (state.dir == Direction::EAST) {
            double offset = state.coor.x - base_.x;
            Move(state.coor.x, base_.x, offset, r_, true, state.area, state.dir, state.speed);
            //std::cerr << "Area after: " << state.area->GetBase();
            //std::cerr << "After: " << state.coor << std::endl;
        } else if (state.dir == Direction::SOUTH) {
            double offset = state.coor.y - base_.y;
            Move(state.coor.y, base_.y, offset, d_, true, state.area, state.dir, state.speed);
            //std::cerr << "Area after: " << state.area->GetBase();
            //std::cerr << "After: " << state.coor << std::endl;
        } else if (state.dir == Direction::WEST) {
            double offset = state.coor.x - base_.x;
            Move(state.coor.x, base_.x, offset, l_, false, state.area, state.dir, state.speed);
            //std::cerr << "Area after: " << state.area->GetBase();
            //std::cerr << "After: " << state.coor << std::endl;
        } else {
            throw std::logic_error("No need to handle NONE direction");
        }
    }
private:
    void Move(double& coor, int base_coor, double offset, Area* near_dot,
              bool positive, const Area*& area, Direction dir, Speed& speed) const {
        int sign = -1 + positive * 2;

        //std::cerr << "\n\nIn " << base_ << std::endl;
        //std::cerr << std::boolalpha << "coor: " << coor << ", base_coor: " << base_coor << ", offset: " << offset
        //          << ", near_dot: " << (near_dot != nullptr) << ", pos: " << positive << std::endl;

        offset = std::abs(offset);

        //std::cerr << "new offset: " << offset << std::endl;


        if (offset <= 0.4 || (offset <= 0.5 && near_dot != nullptr)) {
            //std::cerr << "First if" << std::endl;

            area = this;
            return;
        }
        if (near_dot == nullptr) {
            //std::cerr << "Second if" << std::endl;
            coor = base_coor + 0.4 * sign;
            speed = {0, 0};
            area = this;
            return;        }

        if (offset > 1) {
            area = near_dot;
            offset -= 1;
            base_coor += sign;
            Area* neigh;
            if (dir == Direction::NORTH) {
                neigh = near_dot->u_;
            } else if (dir == Direction::EAST) {
                neigh = near_dot->r_;
            } else if (dir == Direction::SOUTH) {
                neigh = near_dot->d_;
            } else if (dir == Direction::WEST) {
                neigh = near_dot->l_;
            }
            area->Move(coor, base_coor, offset, neigh, positive, area, dir, speed);
            return;
        }

        //std::cerr << "No if" << std::endl;
        area = near_dot;
    }


    model::Point base_;
    Area* u_ = nullptr;
    Area* r_ = nullptr;
    Area* d_ = nullptr;
    Area* l_ = nullptr;
};


class Map {
public:
    using AreaMap = std::unordered_map<model::Point, Area*, PointHasher>;

    Map(const model::Map& map) : Map(map.GetRoads()) {}

    Map(const std::vector<model::Road>& roads) {
        AreaMap areas;

        for (const model::Road& road : roads) {
            if (road.IsHorizontal()) {
                int y = road.GetStart().y;
                int x1 = std::min(road.GetStart().x, road.GetEnd().x);
                int x2 = std::max(road.GetStart().x, road.GetEnd().x);
                for (; x1 <= x2; x1++) {
                    AddArea({x1, y}, areas);
                }
            } else {
                int x = road.GetStart().x;
                int y1 = std::min(road.GetStart().y, road.GetEnd().y);
                int y2 = std::max(road.GetStart().y, road.GetEnd().y);
                for (; y1 <= y2; y1++) {
                    AddArea({x, y1}, areas);
                }
            }
        }
    }

    MoveResult GetStartPlace() const {
        const Area& area = areas_.at(0);
        Coords coords;
        coords.x = area.GetBase().x;
        coords.y = area.GetBase().y;

        return {&areas_.at(0), coords};
    }

    MoveResult GetRandomPlace() const {
        const Area& random_area = GetRandomArea();
        Coords random_coords = GetRandomOffset();
        random_coords.x += random_area.GetBase().x;
        random_coords.y += random_area.GetBase().y;
        return {&random_area, random_coords};
    }

private:
    void AddArea (model::Point point, AreaMap& area_map) {
        if (area_map.find(point) == area_map.end()) {
            areas_.emplace_back(point);
            area_map[point] = &areas_.back();
            AddNeigbours(areas_.back(), area_map);
        }
    }

    void AddNeigbours(Area& area, AreaMap& area_map) {
        model::Point coors = area.GetBase();

        auto u_it = area_map.find({coors.x, coors.y - 1});
        auto r_it = area_map.find({coors.x + 1, coors.y});
        auto d_it = area_map.find({coors.x, coors.y + 1});
        auto l_it = area_map.find({coors.x - 1, coors.y});
        auto end = area_map.end();

        if (u_it != end) {
            area.SetUp(u_it->second);
            u_it->second->SetDown(&area);
        }

        if (r_it != end) {
            area.SetRight(r_it->second);
            r_it->second->SetLeft(&area);
        }

        if (d_it != end) {
            area.SetDown(d_it->second);
            d_it->second->SetUp(&area);
        }

        if (l_it != end) {
            area.SetLeft(l_it->second);
            l_it->second->SetRight(&area);
        }
    }

    const Area& GetRandomArea() const {
        std::random_device rd;
        std::mt19937 generator = std::mt19937(rd());
        std::uniform_int_distribution<int> dis{0, static_cast<int>(areas_.size() - 1)};
        return areas_.at(dis(generator));
    }

    Coords GetRandomOffset() const {
        std::random_device rd;
        std::mt19937 generator = std::mt19937(rd());
        std::uniform_real_distribution<> dis(-0.4, 0.4);
        return {dis(generator), dis(generator)};
    }

    std::deque<Area> areas_;
};
} // namespace move_manager
