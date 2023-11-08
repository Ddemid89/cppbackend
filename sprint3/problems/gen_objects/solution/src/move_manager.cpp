#include "move_manager.h"

namespace move_manager {

size_t PointHasher::operator()(const model::Point& point) const {
    std::hash<int> hasher;
    size_t x_hash = hasher(point.x);
    size_t y_hash = hasher(point.y);
    return x_hash ^ (y_hash << 1);
}


std::string GetStringDirection(Direction dir) {
    using namespace std::literals;
    switch (dir) {
    case Direction::NORTH:
        return "U"s;
    case Direction::EAST:
        return "R"s;
    case Direction::SOUTH:
        return "D"s;
    case Direction::WEST:
        return "L"s;
    default:
        throw std::logic_error("Direction not supported");
    }
}

std::optional<Direction> GetDirectionFromString(std::string_view dir) {
    using namespace std::literals;

    if (dir.empty()) {
        return Direction::NONE;
    }

    if (dir.size() != 1) {
        return std::nullopt;
    }

    switch(dir.at(0)) {
    case 'U':
        return Direction::NORTH;
    case 'D':
        return Direction::SOUTH;
    case 'L':
        return Direction::WEST;
    case 'R':
        return Direction::EAST;
    }
    return std::nullopt;
}


const Area* Area::GetNeighbour(Direction dir) const {
    switch (dir) {
    case Direction::NORTH:
        return u_;
    case Direction::EAST:
        return r_;
    case Direction::SOUTH:
        return d_;
    case Direction::WEST:
        return l_;
    default:
        throw std::logic_error("Area: NONE direction neighbour");
    }
}

bool Area::IsIntersects(Coords coords, Direction dir, bool borders, bool other_axis) const {
    double left_border   = base_.x - 0.4 - 0.1 * (l_ != nullptr || !borders);
    double right_border  = base_.x + 0.4 + 0.1 * (r_ != nullptr || !borders);
    double top_border    = base_.y - 0.4 - 0.1 * (u_ != nullptr || !borders);
    double bottom_border = base_.y + 0.4 + 0.1 * (d_ != nullptr || !borders);

    bool horizontal = (dir == Direction::EAST || dir == Direction::WEST) != other_axis;

    if (dir == Direction::NONE) {
        return left_border <= coords.x && coords.x <= right_border
            && top_border  <= coords.y && coords.y <= bottom_border;
    } else if (horizontal) {
        return left_border <= coords.x && coords.x <= right_border;
    } else {
        return top_border  <= coords.y && coords.y <= bottom_border;
    }
}

double Area::GetMaxCoor(Direction dir) const {
    double max_delta = 0.4;
    if (GetNeighbour(dir)) {
        max_delta = 0.5;
    }

    switch (dir) {
    case Direction::NORTH:
        return base_.y - max_delta;
    case Direction::EAST:
        return base_.x + max_delta;
    case Direction::SOUTH:
        return base_.y + max_delta;
    case Direction::WEST:
        return base_.x - max_delta;
    default:
        throw std::logic_error("Area: NONE direction borders");
    }
}

Coords Area::Place(Coords coor) const {
    coor.x = std::clamp(coor.x, base_.x - 0.4 - 0.1 * (l_ != nullptr), base_.x + 0.4 + 0.1 * (r_ != nullptr));
    coor.y = std::clamp(coor.y, base_.y - 0.4 - 0.1 * (u_ != nullptr), base_.y + 0.4 + 0.1 * (d_ != nullptr));
    return coor;
}

void State::Move(uint64_t dur) {
    Coords target_position;
    target_position.x = position.coor.x + (speed.x_axis * dur) / 1000;
    target_position.y = position.coor.y + (speed.y_axis * dur) / 1000;

    while (!position.area->IsIntersects(target_position, Direction::NONE, false) && position.area->GetNeighbour(dir)) {
        position.area = position.area->GetNeighbour(dir);
    }

    if (!position.area->IsIntersects(target_position, dir)) {
        speed = {0.0, 0.0};
        if (dir == Direction::NORTH || dir == Direction::SOUTH) {
            position.coor.y = position.area->GetMaxCoor(dir);
        } else {
            position.coor.x = position.area->GetMaxCoor(dir);
        }
    } else {
        if (!position.area->IsIntersects(target_position, dir, true, true)) {
            target_position = position.area->Place(target_position);
        }
        position.coor = target_position;

    }
}

void Map::AddRoad(const model::Road& road, AreaMap& area_map) {
    if (road.IsHorizontal()) {
        int y = road.GetStart().y;
        int x1 = std::min(road.GetStart().x, road.GetEnd().x);
        int x2 = std::max(road.GetStart().x, road.GetEnd().x);
        for (; x1 <= x2; x1++) {
            AddArea({x1, y}, area_map);
        }
    } else {
        int x = road.GetStart().x;
        int y1 = std::min(road.GetStart().y, road.GetEnd().y);
        int y2 = std::max(road.GetStart().y, road.GetEnd().y);
        for (; y1 <= y2; y1++) {
            AddArea({x, y1}, area_map);
        }
    }
}

Map::Map(const std::vector<model::Road>& roads) {
    AreaMap areas;

    for (const model::Road& road : roads) {
        AddRoad(road, areas);
    }
}

PositionState Map::GetStartPlace() const {
    const Area& area = areas_.at(0);
    PositionState res;
    res.coor.x = area.GetBase().x;
    res.coor.y = area.GetBase().y;
    res.area = &area;

    return res;
}

PositionState Map::GetRandomPlace() const {
    const Area& random_area = GetRandomArea();
    PositionState res;
    res.coor = GetRandomOffset();
    res.coor.x += random_area.GetBase().x;
    res.coor.y += random_area.GetBase().y;
    res.area = &random_area;

    return res;
}

void Map::AddArea (model::Point point, AreaMap& area_map) {
    if (area_map.find(point) == area_map.end()) {
        areas_.emplace_back(point);
        area_map[point] = &areas_.back();
        AddNeigbours(areas_.back(), area_map);
    }
}

void Map::AddNeigbours(Area& area, AreaMap& area_map) {
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

const Area& Map::GetRandomArea() const {
    std::random_device rd;
    std::mt19937 generator = std::mt19937(rd());
    std::uniform_int_distribution<int> dis{0, static_cast<int>(areas_.size() - 1)};
    return areas_.at(dis(generator));
}

Coords Map::GetRandomOffset() const {
    std::random_device rd;
    std::mt19937 generator = std::mt19937(rd());
    std::uniform_real_distribution<> dis(-0.4, 0.4);
    return {dis(generator), dis(generator)};
}

} // namespace move_manager
