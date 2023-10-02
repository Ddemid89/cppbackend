#pragma once
#include "http_server.h"
#include "model.h"
#include <vector>
#include <string_view>
#include <iostream>
#include <optional>
#include <filesystem>
#include <unordered_map>

#include <boost/json.hpp>
#include <boost/beast.hpp>

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
namespace sys = boost::system;
using HttpResponse = http::response<http::string_body>;
using namespace std::literals;

namespace body_type {
const std::string html = "text/html"s; //.htm, .html: text/html
const std::string css = "text/css"s; //.css: text/css
const std::string txt = "text/plain"s; //.txt: text/plain
const std::string js = "text/javascript"s; //.js: text/javascript
const std::string json = "application/json"s; //.json: application/json
const std::string xml = "application/xml"s; //.xml: application/xml
const std::string png = "image/png"s; //.png: image/png
const std::string jpg = "image/jpeg"s; //.jpg, .jpe, .jpeg: image/jpeg
const std::string gif = "image/gif"s; //.gif: image/gif
const std::string bmp = "image/bmp"s; //.bmp: image/bmp
const std::string ico = "image/vnd.microsoft.icon"s; //.ico: image/vnd.microsoft.icon
const std::string tif = "image/tiff"s; //.tiff, .tif: image/tiff
const std::string svg = "image/svg+xml"s; //.svg, .svgz: image/svg+xml
const std::string mp3 = "audio/mpeg"s; //.mp3: audio/mpeg
const std::string unknown = "application/octet-stream"s; //other

//Любой регистр

std::string to_lower_case(std::string_view str);
std::string_view GetTypeByExtention(std::string_view file);
} // namespace body_type

namespace detail {
enum class Format {
    JSON
};

template <typename Body, typename Allocator>
class ResponseMaker {
public:
    using ResponseType = http::response<Body, http::basic_fields<Allocator>>;
    using FileResponseType = http::response<http::file_body, http::basic_fields<Allocator>>;

    ResponseMaker(http::request<Body, http::basic_fields<Allocator>>& req) : req_(req) {
    }

    ResponseType MakeBadRequestResponse(std::string_view body,
                                        std::string_view content_type) {
        return MakeResponse(body, http::status::bad_request, content_type);
    }

    ResponseType MakeOkResponse(std::string_view body,
                                std::string_view content_type) {
        return MakeResponse(body, http::status::ok, content_type);
    }

    ResponseType MakeNotFoundResponse(std::string_view body,
                                      std::string_view content_type) {
        return MakeResponse(body, http::status::not_found, content_type);
    }

    ResponseType MakeServerErrorResponse(std::string_view body,
                                         std::string_view content_type) {
        return MakeResponse(body, http::status::internal_server_error, content_type);
    }

    FileResponseType MakeFileResponse(const std::filesystem::path& file) {
        http::file_body::value_type data;

        if (sys::error_code ec; data.open(file.string().data(), beast::file_mode::read, ec), ec) {
            throw std::runtime_error("Failed to open file");
        }

        std::string_view content_type = body_type::GetTypeByExtention(file.extension().string());
        FileResponseType result(http::status::ok, req_.version());
        result.keep_alive(req_.keep_alive());
        result.set(http::field::content_type, content_type);

        result.body() = std::move(data);
        result.prepare_payload();

        return result;
    }

private:
    ResponseType MakeResponse(std::string_view body, http::status status,
                              std::string_view content_type) {
        ResponseType result(status, req_.version());
        result.body() = body;
        result.content_length(body.size());
        result.keep_alive(req_.keep_alive());
        result.set(http::field::content_type, content_type);
        return result;
    }

    http::request<Body, http::basic_fields<Allocator>>& req_;
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
    std::string MakeBadRequestBody() override;
    std::string MakeNotFoundBody() override;
    std::string MakeOneMapBody(const model::Map& map) override;
    std::string MakeMapsBody(const std::vector<model::MapInfo>& maps) override;
private:
    std::string GetBadRequestBodyTxt ();
    std::string GetNotFoundBodyTxt ();
};

std::unique_ptr<BodyMaker> GetBodyMaker(Format format);
std::vector<std::string_view> ParseTarget(std::string_view target);
} //namespace detail

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, std::filesystem::path static_data_path)
        : game_{game},
          static_data_path_(static_data_path) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        bool is_get = req.method() == http::verb::get;
        std::string_view target = req.target();

        auto resp_maker = detail::GetResponseMaker(req);

        if (target.starts_with("/api"sv)) { //Константа
            HandleApiResponse(resp_maker, target, std::forward<Send>(send));
        } else {
            HandleStaticDataResponse(resp_maker, target, std::forward<Send>(send));
        }
    }

private:
    template <typename Body, typename Allocator, typename Send>
    void HandleApiResponse(detail::ResponseMaker<Body, Allocator>& resp_maker,
                        std::string_view target, Send&& send) {

        auto body_maker = detail::GetBodyMaker(detail::Format::JSON);

        if (target.starts_with("/api/v1/maps"sv)) { //const
            std::string_view map = target.substr("/api/v1/maps"sv.size());
            if (map == ""sv || map == "/"sv) {
                send(resp_maker.MakeOkResponse(body_maker->MakeMapsBody(game_.GetMapsInfo()), body_type::json));
            } else {
                if (map.at(0) == '/') {
                    map = map.substr(1);
                    if (map.back() == '/') {
                        map = map.substr(0, map.size() - 1);
                    }
                    if (map.find('/') == map.npos) {
                        model::Map::Id map_id{std::string(map)};
                        auto map_ptr = game_.FindMap(map_id);
                        if (map_ptr != nullptr) {
                            send(resp_maker.MakeOkResponse(body_maker->MakeOneMapBody(*map_ptr), body_type::json));
                        } else {
                            send(resp_maker.MakeNotFoundResponse(body_maker->MakeNotFoundBody(), body_type::json));
                        }
                    } else {
                        send(resp_maker.MakeBadRequestResponse(body_maker->MakeBadRequestBody(), body_type::json));
                    }
                } else {
                    send(resp_maker.MakeBadRequestResponse(body_maker->MakeBadRequestBody(), body_type::json));
                }
            }
        } else {
            send(resp_maker.MakeBadRequestResponse(body_maker->MakeBadRequestBody(), body_type::json));
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleStaticDataResponse(detail::ResponseMaker<Body, Allocator>& resp_maker,
                               std::string_view target, Send&& send) {

        namespace fs = std::filesystem;
        fs::path file = fs::weakly_canonical(static_data_path_ / target.substr(1));

        if (!file.string().starts_with(static_data_path_.string())) {
            send(resp_maker.MakeBadRequestResponse("Access denied", body_type::txt));
            return;
        }

        if (fs::exists(file)) {
            if (fs::is_directory(file)) {
                if (fs::exists(file / "index.html")) {
                    file /= "index.html";
                    TryToSendFile(resp_maker, file, send);
                } else if (fs::exists(file / "index.htm")){
                    file /= "index.htm";
                    TryToSendFile(resp_maker, file, send);
                } else {
                    send(resp_maker.MakeNotFoundResponse("No index.html in derictory", body_type::txt));
                }
            } else {
                TryToSendFile(resp_maker, file, send);
            }
        } else {
            send(resp_maker.MakeNotFoundResponse("No such file or directory", body_type::txt));
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void TryToSendFile(detail::ResponseMaker<Body, Allocator>& resp_maker,
                       const std::filesystem::path& file, Send&& send) {
        try {
            send(resp_maker.MakeFileResponse(file));
        } catch (std::runtime_error& e) {
            send(resp_maker.MakeServerErrorResponse(e.what(), body_type::txt));
        } catch (...) {
            send(resp_maker.MakeServerErrorResponse("Unknown error", body_type::txt));
        }
    }

    std::filesystem::path static_data_path_;
    model::Game& game_;
};

}  // namespace http_handler
