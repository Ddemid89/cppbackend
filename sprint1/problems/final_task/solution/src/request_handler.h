#pragma once
#include "http_server.h"
#include "model.h"
#include <vector>
#include <string_view>
#include <iostream>

#include <boost/json.hpp>

namespace json = boost::json;

namespace model {
void tag_invoke (json::value_from_tag, json::value& jv, const Road& road);

void tag_invoke (json::value_from_tag, json::value& jv, const Building& building);

void tag_invoke (json::value_from_tag, json::value& jv, const Office& office);

void tag_invoke (json::value_from_tag, json::value& jv, const Map& map);

void tag_invoke (json::value_from_tag, json::value& jv, const MapInfo& map);
} // namespace model


namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
using HttpResponse = http::response<http::string_body>;
using namespace std::literals;

namespace  {
enum class Format {
    JSON
};

template <typename Body, typename Allocator>
class ResponseMaker {
public:
    using ResponseType = http::response<Body, http::basic_fields<Allocator>>;
    ResponseMaker(http::request<Body, http::basic_fields<Allocator>>& req) : req_(req) {}

    ResponseType MakeBadRequestResponse(const std::string& body) {
        return MakeResponse(body, http::status::bad_request);
    }

    ResponseType MakeOkResponse(const std::string& body) {
        return MakeResponse(body, http::status::ok);
    }

    ResponseType MakeNotFoundResponse(const std::string& body) {
        return MakeResponse(body, http::status::not_found);
    }

private:
    ResponseType MakeResponse(const std::string& body, http::status status) {
        ResponseType result(status, req_.version());
        result.body() = body;
        result.content_length(body.size());
        result.keep_alive(req_.keep_alive());
        result.set(http::field::content_type, content_type_);
        return result;
    }

    http::request<Body, http::basic_fields<Allocator>>& req_;
    const std::string content_type_ = "application/json";
};

template <typename Body, typename Allocator>
ResponseMaker<Body, Allocator>
GetResponseMaker(http::request<Body, http::basic_fields<Allocator>>& req) {
    return ResponseMaker<Body, Allocator>(req);
}

class BodyMaker{
public:
    virtual std::string MakeBadRequestBody() = 0;
    virtual std::string MakeNotFoundBody() = 0;
    virtual std::string MakeOneMapBody(const model::Map&) = 0;
    virtual std::string MakeMapsBody(const std::vector<model::MapInfo>&) = 0;
    virtual ~BodyMaker() = default;
};

class JsonBodyMaker : public BodyMaker{
public:
    std::string MakeBadRequestBody() override {
        static std::string result = GetBadRequestBodyTxt();
        return result;
    }

    std::string MakeNotFoundBody() override {
        static std::string result = GetNotFoundBodyTxt();
        return result;
    }

    std::string MakeOneMapBody(const model::Map& map) override {
        return json::serialize(json::value_from(map));
    }

    std::string MakeMapsBody(const std::vector<model::MapInfo>& maps) override {
        return json::serialize(json::value_from(maps));
    }
private:
    std::string GetBadRequestBodyTxt () {
        json::value jv = {
            {"code", "badRequest"},
            {"message", "Bad request"}
        };
        return json::serialize(jv);
    }

    std::string GetNotFoundBodyTxt () {
        json::value jv = {
            {"code", "mapNotFound"},
            {"message", "Map not found"}
        };
        return json::serialize(jv);
    }
};

std::unique_ptr<BodyMaker> GetBodyMaker(Format format) {
    if (format == Format::JSON) {
        return std::make_unique<JsonBodyMaker>();
    }
    throw std::logic_error("Type not available!");
}

std::vector<std::string_view> ParseTarget(std::string_view target) {
    std::vector<std::string_view> result;

    size_t start = target.find_first_not_of('/');
    size_t end = target.find_first_of('/', start);

    while (end != target.npos) {
        result.emplace_back(target.substr(start, end - start));
        start = target.find_first_not_of('/', end);
        end = target.find_first_of('/', start);
    }

    if (start != target.npos) {
        result.emplace_back(target.substr(start));
    }

    return result;
}
} //namespace

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        bool is_get = req.method() == http::verb::get;
        const auto& target = req.target();

        auto parsed_target = ParseTarget(target);

        Format format = Format::JSON;

        auto resp_maker = GetResponseMaker(req);
        std::unique_ptr<BodyMaker> body_maker = GetBodyMaker(format);

        if (!is_get || parsed_target.size() < 3 || parsed_target.size() > 4) {
            send(resp_maker.MakeBadRequestResponse(body_maker->MakeBadRequestBody()));
            return;
        }

        if (parsed_target.at(0) != "api"s
            || parsed_target.at(1) != "v1"s
            || parsed_target.at(2) != "maps"s) {
            send(resp_maker.MakeBadRequestResponse(body_maker->MakeBadRequestBody()));
            return;
        }

        if (parsed_target.size() == 3) {
            send(resp_maker.MakeOkResponse(body_maker->MakeMapsBody(game_.GetMapsInfo())));
            return;
        }

        model::Map::Id map_id(std::string(parsed_target.at(3)));

        auto map = game_.FindMap(map_id);

        if (map == nullptr) {
            send(resp_maker.MakeNotFoundResponse(body_maker->MakeNotFoundBody()));
            return;
        } else {
            send(resp_maker.MakeOkResponse(body_maker->MakeOneMapBody(*map)));
            return;
        }
    }

private:
    model::Game& game_;
};

}  // namespace http_handler
