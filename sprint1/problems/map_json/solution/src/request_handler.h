#pragma once
#include "http_server.h"
#include "model.h"
#include <vector>
#include <string_view>
#include <iostream>


namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
using HttpResponse = http::response<http::string_body>;
using namespace std::literals;

namespace  {
std::vector<std::string_view> ParseTarget(std::string_view target) {
    std::vector<std::string_view> result;
    size_t start = 0;

    for (int i = 0; i < target.size(); ++i) {
        if (target.at(i) == '/') {
            if (start != i) {
                result.emplace_back(target.substr(start, i - start));
            }
            start = i + 1;
        }
    }

    if (start != target.size()) {
        result.emplace_back(target.substr(start));
    }

    return result;
}

} //namespace

class MapConverter{
public:
    virtual std::string ConvertOneMap(const model::Map&) = 0;
    virtual std::string ConvertRoad(const model::Road&) = 0;
    virtual std::string ConvertOneMapShort(const model::Map&) = 0;

    //virtual HttpResponse ConvertSeveralMaps(model::Map) = 0;
protected:
    ~MapConverter() = default;
};

class MapJsonConverter : public MapConverter{
public:
    std::string ConvertRoad(const model::Road& road) override {
        std::string res = "{\"x0\": ";
        res.append(std::to_string(road.GetStart().x));
        res.append(", \"y0\": ");
        res.append(std::to_string(road.GetStart().y));

        if (road.GetStart().x == road.GetEnd().x) {
            res.append(", \"y1\": ");
            res.append(std::to_string(road.GetEnd().y));
        } else {
            res.append(", \"x1\": ");
            res.append(std::to_string(road.GetEnd().x));
        }

        res.append("}");

        return res;
    }

    std::string ConvertOneMapShort(const model::Map& map) override {
        std::string res = "{\"id\": \"";
        res.append(*map.GetId());
        res.append("\", \"name\": \"");
        res.append(map.GetName());
        res.append("\"}");

        return res;
    }

    std::string ConvertOneMap(const model::Map& map) override {
        std::string result = "{\n\t\"id\": \""s.append(*map.GetId());
        result.append("\",\n\t\"name\": \""s).append(map.GetName());
        result.append("\",\n\t\"roads\": [\n\t\t"s);

        bool is_first = true;

        for (auto& road : map.GetRoads()) {
            if (!is_first) {
                result.append(",\n\t\t");
            }
            is_first = false;
            result.append(ConvertRoad(road));
        }
        result.append("\n\t],\n\t\"buildings\": [\n\t\t");

        is_first = true;
        for (auto& building : map.GetBuildings()) {
            if (!is_first) {
                result.append(",\n\t\t");
            }
            is_first = false;

            result.append(ConvertBuilding(building));
        }

        result.append("\n\t],\n\t\"offices\": [\n\t\t");

        is_first = true;
        for (auto& office : map.GetOffices()) {
            if (!is_first) {
                result.append(",\n\t\t");
            }
            is_first = false;

            result.append(ConvertOffice(office));
        }
        result.append("\n\t]\n}");

        return result;
    }
private:
    std::string ConvertBuilding(const model::Building& building) {
        std::string result;

        result.append("{\"x\": ");
        result.append(std::to_string(building.GetBounds().position.x));
        result.append(", \"y\": ");
        result.append(std::to_string(building.GetBounds().position.y));
        result.append(", \"w\": ");
        result.append(std::to_string(building.GetBounds().size.width));
        result.append(", \"h\": ");
        result.append(std::to_string(building.GetBounds().size.height));
        result.append("}");

        return result;
    }

    std::string ConvertOffice(const model::Office& office) {
        std::string result;

        result.append("{\"id\": \"");
        result.append(*office.GetId());
        result.append("\", \"x\": ");
        result.append(std::to_string(office.GetPosition().x));
        result.append(", \"y\": ");
        result.append(std::to_string(office.GetPosition().y));
        result.append(", \"offsetX\": ");
        result.append(std::to_string(office.GetOffset().dx));
        result.append(", \"offsetY\": ");
        result.append(std::to_string(office.GetOffset().dy));
        result.append("}");

        return result;
    }
};



class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        using HttpRequest = http::request<http::string_body>;
        HttpRequest req2;
        bool is_get = req.method() == http::verb::get;
        const auto& target = req.target();

        auto parsed_target = ParseTarget(target);

        HttpResponse response;

        if (is_get) {
            if (!parsed_target.empty()) {
                if(parsed_target.at(0) == "api") {
                    response = HandleApiRequest(parsed_target, 1);
                } else {
                    //Пока поддерживаются запросы только к /api/
                    response = GetBadRequestResponse();
                }
            } else {
                //Пустые запросы не поддерживаются
                response = GetBadRequestResponse();
            }
        } else {
            //Пока не поддерживается ничего, кроме GET
            response = GetBadRequestResponse();
        }

        response.version(req.version());
        response.set(http::field::content_type, "application/json");
        response.keep_alive(req.keep_alive());
        send(response);
    }

private:
    HttpResponse HandleApiRequest(const std::vector<std::string_view>& target, size_t level) {
        if (target.size() > level) {
            if (target.at(level) == "v1") {
                return HandleV1Request(target, level + 1);
            } else {
                //Поддерживаются только запросы к /api/v1/
                return GetBadRequestResponse();
            }
        } else {
            //Не поддерживаются запросы просто к /api/
            return GetBadRequestResponse();
        }
    }

    HttpResponse HandleV1Request(const std::vector<std::string_view>& target, size_t level) {
        if (target.size() > level) {
            if (target.at(level) == "maps") {
                return HandleMapsRequest(target, level + 1);
            } else {
                //Поддерживаются запросы только к /api/v1/maps
                return GetBadRequestResponse();
            }
        } else {
            //Не поддерживаются запросы просто к /api/v1/
            return GetBadRequestResponse();
        }
    }

    HttpResponse HandleMapsRequest(const std::vector<std::string_view>& target, size_t level) {
        if (target.size() > level) {
            if (target.size() == level + 1) {
                return HandleOneMapResponse(target.at(level));
            } else {
                // Не поддерживается .../maps/some_map/...
                return GetBadRequestResponse();
            }
        } else {
            return HandleAllMapsResponse();
        }
    }

    HttpResponse HandleAllMapsResponse() {
        MapJsonConverter converter;
        HttpResponse maps_response;

        auto& maps = game_.GetMaps();
        std::string body = "[\n\t";

        bool is_first = true;

        for (auto& map : maps) {
            if (!is_first) {
                body.append(",\n\t");
            }
            is_first = false;
            body.append(converter.ConvertOneMapShort(map));
        }

        body.append("\n]");

        maps_response.body() = body;
        maps_response.content_length(maps_response.body().size());
        maps_response.result(http::status::ok);
        return maps_response;
    }

    HttpResponse HandleOneMapResponse(std::string_view map) {
        model::Map::Id map_id{std::string(map)};
        auto map_ptr = game_.FindMap(map_id);
        MapJsonConverter converter;

        if (map_ptr != nullptr) {
            HttpResponse map_response;
            map_response.body() = converter.ConvertOneMap(*map_ptr);
            map_response.content_length(map_response.body().size());
            map_response.result(http::status::ok);
            return map_response;
        } else {
            HttpResponse not_found;
            not_found.body() = "{ \n\t\"code\": \"mapNotFound\", \n\t\"message\": \"Map not found\"\n}";
            not_found.content_length(not_found.body().size());
            not_found.result(http::status::not_found);
            return not_found;
        }
    }

    HttpResponse MakeBadRequestResponse() {
        HttpResponse result;
        result.body() = "{\n\t\"code\": \"badRequest\",\n\t\"message\": \"Bad request\"\n}";
        result.content_length(result.body().size());
        result.result(http::status::bad_request);
        return result;
    }

    const HttpResponse& GetBadRequestResponse() {
        const static HttpResponse bad_request_response = MakeBadRequestResponse();
        return bad_request_response;
    }

    model::Game& game_;
};

}  // namespace http_handler
