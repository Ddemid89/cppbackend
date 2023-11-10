#define _USE_MATH_DEFINES

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "../src/collision_detector.h"
#include "../src/geom.h"

#include <vector>
#include <random>

const double DOG_WIDTH = 0.3;
const double EPSILON = 10E-10;

class TestProvider : public collision_detector::ItemGathererProvider {
public:
    void AddItem (collision_detector::Item item) {
        items_.push_back(item);
    }
    void AddGetherer(collision_detector::Gatherer gatherer) {
        gatherers_.push_back(gatherer);
    }
    size_t ItemsCount() const override {
        return items_.size();
    }
    collision_detector::Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }
    size_t GatherersCount() const override {
        return gatherers_.size();
    }
    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }
private:
    std::vector<collision_detector::Item> items_;
    std::vector<collision_detector::Gatherer> gatherers_;
};

// Напишите здесь тесты для функции collision_detector::FindGatherEvents

void CheckCollisions(geom::Point2D start, geom::Point2D finish, const double WIDTH) {
    using namespace collision_detector;
    using Catch::Matchers::Contains;

    TestProvider prov;

    Gatherer dog1;
    dog1.start_pos = start;
    dog1.end_pos   = finish;
    dog1.width     = WIDTH;

    prov.AddGetherer(dog1);

    double min_x = std::min(start.x, finish.x);
    double max_x = std::max(start.x, finish.x);
    double min_y = std::min(start.y, finish.y);
    double max_y = std::max(start.y, finish.y);

    double hor_width = 0;
    double ver_width = 0;

    if (min_x == max_x) {
        hor_width = WIDTH;
    } else {
        ver_width = WIDTH;
    }

    const std::vector<double> horizontal_positions = {min_x - hor_width - 1,     //-
                                                      min_x - hor_width - 0.01,  //-
                                                      min_x - hor_width + 0.01,  //+
                                                     (min_x + max_x) / 2,        //+
                                                      max_x + hor_width - 0.01,  //+
                                                      max_x + hor_width  + 0.01, //-
                                                      max_x + hor_width  + 1     //-
                                                     }; // total: 3


    const std::vector<double> vertical_positions = {min_y - ver_width - 1,    //-
                                                    min_y - ver_width - 0.01, //-
                                                    min_y - ver_width + 0.01, //+
                                                   (min_y + max_y) / 2,       //+
                                                    max_y + ver_width - 0.01, //+
                                                    max_y + ver_width + 0.01, //-
                                                    max_y + ver_width + 1     //-
                                                   }; // total: 3

    for (int i = 0; i < vertical_positions.size(); ++i) {       //  0-  1-  2-  3-  4-  5-  6-
        for (int j = 0; j < horizontal_positions.size(); ++j) { //  7-  8-  9- 10- 11- 12- 13-
            Item new_item;                                      // 14- 15- 16+ 17+ 18+ 19- 20-
            new_item.position.y = vertical_positions.at(i);     // 21- 22- 23+ 24+ 25+ 26- 27-
            new_item.position.x = horizontal_positions.at(j);   // 28- 29- 30+ 31+ 32+ 33- 34-
            new_item.width = 0;                                 // 35- 36- 37- 38- 39- 40- 41-
            prov.AddItem(new_item);                             // 42- 43- 44- 45- 46- 47- 48-
        }
    }

    const std::vector<size_t> coll_indxs = {16, 17, 18, 23, 24, 25, 30, 31, 32};

    auto result = FindGatherEvents(prov);
    REQUIRE(result.size() == 9);

    for(int i = 0; i < coll_indxs.size(); ++i) {
        CHECK_THAT(coll_indxs, Contains(result.at(i).item_id));
    }

}

geom::Point2D GetRandomPoint() {
    static std::random_device rd;

    std::uniform_real_distribution<> dist(-10., 10.);

    return {dist(rd), dist(rd)};
}

std::vector<collision_detector::Item> MakeRandomPointsWithCollision
(geom::Point2D start, geom::Point2D finish, const double WIDTH, size_t items_number)
{
    static std::random_device rd;

    REQUIRE(items_number != 0);
    REQUIRE((start.x != finish.x || start.y != finish.y));

    double min_x = std::min(start.x, finish.x);
    double max_x = std::max(start.x, finish.x);
    double min_y = std::min(start.y, finish.y);
    double max_y = std::max(start.y, finish.y);

    double left_b, right_b, top_b, bottom_b;

    if (min_x == max_x) {
        top_b    = min_y;
        bottom_b = max_y;
        left_b   = min_x - WIDTH + 0.001;
        right_b  = min_x + WIDTH - 0.001;
    } else {
        left_b   = min_x + 0.001;
        right_b  = max_x - 0.001;
        top_b    = min_y - WIDTH + 0.001;
        bottom_b = max_y + WIDTH - 0.001;
    }

    std::uniform_real_distribution<> x_dist(left_b, right_b);
    std::uniform_real_distribution<> y_dist(top_b, bottom_b);

    std::vector<collision_detector::Item> result;
    result.reserve(items_number);

    for (int i = 0; i < items_number; ++i) {
        result.push_back({{x_dist(rd), y_dist(rd)}, 0});
    }

    return result;
}

void CheckDistance(geom::Point2D start, geom::Point2D finish, const double WIDTH, size_t items_number) {
    using namespace collision_detector;
    using Catch::Matchers::WithinAbs;

    Gatherer dog1;
    dog1.start_pos = start;
    dog1.end_pos   = finish;
    dog1.width     = WIDTH;

    TestProvider prov;

    prov.AddGetherer(dog1);

    auto items = MakeRandomPointsWithCollision(start, finish, WIDTH, items_number);

    std::vector<double> distances;
    distances.reserve(items_number);

    double path_x = finish.x - start.x;
    double path_y = finish.y - start.y;

    for (int i = 0; i < items_number; ++i) {
        prov.AddItem(items.at(i));

        double item_x = items.at(i).position.x - start.x;
        double item_y = items.at(i).position.y - start.y;

        double s_mult = item_x * path_x + item_y * path_y;

        double path_sq = path_x * path_x + path_y * path_y;
        double item_sq = item_x * item_x + item_y * item_y;

        distances.push_back(item_sq - (s_mult * s_mult) / path_sq);
    }


    auto result = FindGatherEvents(prov);

    REQUIRE(result.size() == items_number);

    for (int i = 0; i < items_number; ++i) {
        auto& res = result.at(i);
        size_t ind = res.item_id;
        CHECK_THAT(res.sq_distance, WithinAbs(distances.at(ind), EPSILON));
    }
}

void CheckGatherers(size_t dogs_number, size_t items_number, double x_step, double y_step, double width) {
    using namespace collision_detector;

    REQUIRE((x_step == 0 || y_step == 0));
    REQUIRE((std::abs(x_step) > width * 2 || std::abs(y_step) > width * 2));

    TestProvider prov;

    int sign = x_step + y_step > 0 ? 1 : -1;

    double fin_coor = static_cast<double>(items_number) * sign + sign;

    bool item_step_y = y_step == 0; // Значит, что Y между айтемами
    bool item_step_x = x_step == 0;


    for (int i = 0; i < dogs_number; ++i) {
        geom::Point2D start{x_step * i, y_step * i};
        geom::Point2D finish{start.x + fin_coor * item_step_x, start.y + fin_coor * item_step_y};
        prov.AddGetherer({start, finish, width});

        for (int j = 0; j < items_number; ++j) {
            double x = x_step * i + item_step_x * (j + 1) * sign;
            double y = y_step * i + item_step_y * (j + 1) * sign;
            prov.AddItem({{x, y}, 0});
        }
    }

    auto result = FindGatherEvents(prov);

    REQUIRE(result.size() == dogs_number * items_number);

    for (int i = 0; i < result.size(); ++i) {
        CHECK(result.at(i).gatherer_id == result.at(i).item_id / items_number);
    }

}

void CheckOrder(size_t items_number, double x_step, double y_step, double width) {
    REQUIRE((x_step == 0 || y_step == 0));
    REQUIRE((std::abs(x_step) > width * 2 || std::abs(y_step) > width * 2));

    TestProvider prov;

    geom::Point2D start = GetRandomPoint();
    geom::Point2D finish;
    finish.x = start.x + (items_number + 1.) * x_step;
    finish.y = start.y + (items_number + 1.) * y_step;

    prov.AddGetherer({start, finish, width});

    for (int i = 0; i < items_number; ++i) {
        double x = start.x + (i + 1) * x_step;
        double y = start.y + (i + 1) * y_step;
        prov.AddItem({{x, y}, 0});
    }

    auto result = FindGatherEvents(prov);

    REQUIRE(result.size() == items_number);

    for (int i = 0; i < items_number; ++i) {
        CHECK(result.at(i).item_id == i);
    }
}

void CheckTwoGatherersOrder(size_t items_number, double x_step, double y_step, double width) {
    REQUIRE((x_step == 0 || y_step == 0));
    REQUIRE((std::abs(x_step) > width * 2 || std::abs(y_step) > width * 2));

    TestProvider prov;

    geom::Point2D start = GetRandomPoint();
    geom::Point2D finish;
    finish.x = start.x + (items_number + .5) * x_step;
    finish.y = start.y + (items_number + .5) * y_step;

    prov.AddGetherer({start, finish, width});
    prov.AddGetherer({finish, start, width});

    for (int i = 0; i < items_number; ++i) {
        double x = start.x + (i + .5) * x_step;
        double y = start.y + (i + .5) * y_step;
        prov.AddItem({{x, y}, 0});
    }

    auto result = FindGatherEvents(prov);

    REQUIRE(result.size() == items_number * 2);

    for (int i = 0; i < items_number; ++i) {
        CHECK(result.at(i).gatherer_id % 2 == i % 2);
    }
}

TEST_CASE("CheckCollisions") {
    CheckCollisions({1., 1.},  {5., 1.},  DOG_WIDTH);
    CheckCollisions({10., 1.}, {5., 1.},  DOG_WIDTH);
    CheckCollisions({4., 4.},  {4., 11.}, DOG_WIDTH);
    CheckCollisions({4., 15.}, {4., 3.},  DOG_WIDTH);
}

TEST_CASE("CheckDistance") {
    CheckDistance({2., 3.},  {5., 3.},  DOG_WIDTH, 100);
    CheckDistance({12., 3.}, {5., 3.},  DOG_WIDTH, 100);
    CheckDistance({5., 3.},  {5., 13.}, DOG_WIDTH, 100);
    CheckDistance({5., 3.},  {5., -3.}, DOG_WIDTH, 100);
}

TEST_CASE("CheckGatherers") {
    CheckGatherers(5, 10,  1,  0, DOG_WIDTH);
    CheckGatherers(5, 10, -1,  0, DOG_WIDTH);
    CheckGatherers(5, 10,  0,  1, DOG_WIDTH);
    CheckGatherers(5, 10,  0, -1, DOG_WIDTH);
}

TEST_CASE("CheckOrder") {
    CheckOrder(100,  1,  0, DOG_WIDTH);
    CheckOrder(100, -1,  0, DOG_WIDTH);
    CheckOrder(100,  0,  1, DOG_WIDTH);
    CheckOrder(100,  0, -1, DOG_WIDTH);
}

TEST_CASE("ChecOrderTwoGatherers") {
    CheckTwoGatherersOrder(100, 1, 0, DOG_WIDTH);
    CheckTwoGatherersOrder(100, 0, 1, DOG_WIDTH);
}
