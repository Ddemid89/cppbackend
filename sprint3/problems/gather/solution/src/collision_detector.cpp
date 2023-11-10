#include "collision_detector.h"
#include <cassert>

namespace collision_detector {

const double DOG_WIDTH   = .3;
const double ITEM_RADIUS = 0.;


CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    // Проверим, что перемещение ненулевое.
    // Тут приходится использовать строгое равенство, а не приближённое,
    // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
    // расстояние.
    assert(b.x != a.x || b.y != a.y);
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult{sq_distance, proj_ratio};
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> result;
    for (int g = 0; g < provider.GatherersCount(); ++g) {
        const auto& gatherer = provider.GetGatherer(g);
        if (gatherer.start_pos != gatherer.end_pos) {
            for (int i = 0; i < provider.ItemsCount(); ++i) {
                const auto& item = provider.GetItem(i);
                auto res = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);
                if (res.IsCollected(DOG_WIDTH + ITEM_RADIUS)) {
                    GatheringEvent event;
                    event.gatherer_id = g;
                    event.item_id     = i;
                    event.sq_distance = res.sq_distance;
                    event.time        = res.proj_ratio;
                    result.push_back(event);
                }
            }
        }
    }

    std::sort(result.begin(), result.end(), [](const GatheringEvent& a, const GatheringEvent& b){
        return a.time < b.time;
    });
    return result;
}

}  // namespace collision_detector
