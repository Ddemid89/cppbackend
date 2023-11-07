#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>
#include "logger.h"

namespace http_server {
void ReportError(beast::error_code ec, std::string_view what) {
    json_logger::JsonLogger::GetInstance().LogError(what, ec);
    std::cerr << what << ": "sv << ec.message() << std::endl;
}
void SessionBase::Run() {
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
    if (ec) {
        return ReportError(ec, "write"sv);
    }

    if (close) {
        // Семантика ответа требует закрыть соединение
        return Close();
    }

    // Считываем следующий запрос
    Read();
}

void SessionBase::Read() {
    using namespace std::literals;
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    http::async_read(stream_, buffer_, request_,
                     // По окончании операции будет вызван метод OnRead
                     beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    using namespace std::literals;
    if (ec == http::error::end_of_stream) {
        // Нормальная ситуация - клиент закрыл соединение
        return Close();
    }
    if (ec) {
        return ReportError(ec, "read"sv);
    }
    HandleRequest(std::move(request_));
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

}  // namespace http_server

namespace url_decode {
int IntFromHex(char hex) {
    if ('0' <= hex && hex <= '9') {
        return hex - '0';
    } else if ('A' <= hex && hex <= 'F') {
        return hex - 'A' + 10;
    } else if ('a' <= hex && hex <= 'f') {
        return hex - 'a' + 10;
    } else {
        return -1;
    }
}

int IntFrom2Hex(char first, char second) {
    return IntFromHex(first) * 16 + IntFromHex(second);
}

std::string DecodeURL(std::string_view url) {
    size_t size = url.size();
    size_t i = 0;
    std::string result;
    result.reserve(url.size());

    while (i < size) {
        char cur = url.at(i);
        if (cur == '%') {
            if (i > size - 3) {
                throw std::runtime_error("Incorrect url");
            }
            int hex = IntFrom2Hex(url.at(i + 1), url.at(i + 2));
            if (hex < 0) {
                throw std::runtime_error("Incorrect hex number");
            }
            result.push_back(static_cast<char>(hex));
            i += 2;
        } else if (cur == '+') {
            result.push_back(' ');
        } else {
            result.push_back(cur);
        }
        i++;
    }

    return result;
}
} // namespace url_decode
