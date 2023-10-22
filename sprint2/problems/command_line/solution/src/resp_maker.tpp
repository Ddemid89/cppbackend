namespace resp_maker {
namespace detail {

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeTextResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                 ResponseInfo& resp_info)
{
    http::response<Body, http::basic_fields<Allocator>>  result(resp_info.status, req.version());
    result.body() = resp_info.body;
    result.content_length(result.body().size());
    result.keep_alive(req.keep_alive());
    result.set(http::field::content_type, resp_info.content_type);

    if (resp_info.no_cache) {
        result.set(http::field::cache_control, "no-cache");
    }

    for (auto& [key, value] : resp_info.additional_fields) {
        result.set(key, value);
    }

    return result;
}
} // namespace detail
namespace file_resp {
template <typename Body, typename Allocator>
http::response<http::file_body, http::basic_fields<Allocator>>
MakeFileResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                 const std::filesystem::path& file)
{
    using FileResponseType = http::response<http::file_body, http::basic_fields<Allocator>>;
    http::file_body::value_type data;

    if (sys::error_code ec; data.open(file.string().data(), beast::file_mode::read, ec), ec) {
        throw std::runtime_error("Failed to open file");
    }

    std::string_view content_type = body_type::GetTypeByExtention(file.extension().string());
    FileResponseType result(http::status::ok, req.version());
    result.keep_alive(req.keep_alive());
    result.set(http::field::content_type, content_type);

    result.body() = std::move(data);
    result.prepare_payload();

    return result;
}
} // namespace file_resp
namespace txt_resp {
template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeBadRequestResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache)
{
    detail::ResponseInfo info;

    info.body = message;
    info.content_type = body_type::txt;
    info.no_cache = no_cache;
    info.status = http::status::bad_request;

    return detail::MakeTextResponse(req, info);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeNotFoundResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache)
{
    detail::ResponseInfo info;

    info.body = message;
    info.content_type = body_type::txt;
    info.no_cache = no_cache;
    info.status = http::status::not_found;

    return detail::MakeTextResponse(req, info);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeServerErrorResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache)
{
    detail::ResponseInfo info;

    info.body = message;
    info.content_type = body_type::txt;
    info.no_cache = no_cache;
    info.status = http::status::internal_server_error;

    return detail::MakeTextResponse(req, info);
}
} // namespace txt_resp

namespace json_resp {
template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeBadRequestResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, std::string code, bool no_cache) {
    using namespace json_keys;

    detail::ResponseInfo info;

    info.body = GetBadRequestResponseBody(std::move(message), std::move(code));
    info.content_type = body_type::json;
    info.no_cache = no_cache;
    info.status = http::status::bad_request;

    return detail::MakeTextResponse(req, info);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeBadRequestResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, bool no_cache) {
    return MakeBadRequestResponse(req, std::move(message), ""s, no_cache);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeNotFoundResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, std::string code, bool no_cache) {
    using namespace json_keys;

    detail::ResponseInfo info;

    info.body = GetNotFoundResponseBody(std::move(message), std::move(code));
    info.content_type = body_type::json;
    info.no_cache = no_cache;
    info.status = http::status::not_found;

    return detail::MakeTextResponse(req, info);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeOkResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string body, bool no_cache) {
    using namespace json_keys;

    detail::ResponseInfo info;

    info.body = std::move(body);
    info.content_type = body_type::json;
    info.no_cache = no_cache;
    info.status = http::status::ok;

    return detail::MakeTextResponse(req, info);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeServerErrorResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string body, bool no_cache) {
    using namespace json_keys;

    detail::ResponseInfo info;

    info.body = std::move(body);
    info.content_type = body_type::json;
    info.no_cache = no_cache;
    info.status = http::status::internal_server_error;

    return detail::MakeTextResponse(req, info);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeMethodNotAllowedResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string allowed, std::string message, bool no_cache) {
    using namespace json_keys;

    detail::ResponseInfo info;

    std::string_view mess;
    if (message == "") {
        mess = invalid_method_message_post;
    } else {
        mess = message;
    }

    json::value jv = {
        {code_key, invalid_method_key},
        {message_key, mess}
    };

    info.body = json::serialize(jv);
    info.content_type = body_type::json;
    info.no_cache = no_cache;
    info.status = http::status::method_not_allowed;

    info.additional_fields.emplace_back(http::field::allow, allowed);

    return detail::MakeTextResponse(req, info);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeUnauthorizedResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, std::string code, bool no_cache) {
    using namespace json_keys;

    detail::ResponseInfo info;

    std::string_view code_, mess_;
    if (code == ""s) {
        code_ = bad_token_key;
    } else {
        code_ = code;
    }
    if (message == ""s) {
        mess_ = bad_token_mess;
    } else {
        mess_ = message;
    }

    json::value jv = {
        {code_key, code_},
        {message_key, mess_}
    };


    info.body = json::serialize(jv);
    info.content_type = body_type::json;
    info.no_cache = no_cache;
    info.status = http::status::unauthorized;

    return detail::MakeTextResponse(req, info);
}

template <typename Body, typename Allocator>
http::response<Body, http::basic_fields<Allocator>>
MakeUnknownErrorResponse(const http::request<Body, http::basic_fields<Allocator>>& req,
                       std::string message, std::string code, bool no_cache) {
    using namespace json_keys;

    detail::ResponseInfo info;

    json::value jv = {
        {code_key, code},
        {message_key, message}
    };

    info.body = json::serialize(jv);

    info.content_type = body_type::json;
    info.no_cache = no_cache;
    info.status = http::status::unknown;

    return detail::MakeTextResponse(req, info);
}
} // namespace json_resp
} // namespace resp_maker
