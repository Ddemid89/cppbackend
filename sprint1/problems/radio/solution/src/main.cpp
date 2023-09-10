#include "audio.h"
#include <iostream>

#include <boost/asio.hpp>
#include <array>//-------------------------------------
#include <string>
#include <string_view>
#include <regex>

namespace net = boost::asio;
// TCP больше не нужен, импортируем имя UDP
using net::ip::udp;

using namespace std::literals;

void as_client();
void as_server();

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: "sv << argv[0] << " <server|client>"sv << std::endl;
        return 1;
    }

    if (argv[1] == "server"sv) {
        as_server();
    } else if (argv[1] == "client"sv) {
        as_client();
    } else {
        std::cout << "Usage: "sv << argv[0] << " <server|client>"sv << std::endl;
        return 1;
    }

}

bool is_correct_ip(const std::string& txt) {
    static const std::string one_num = R"((25[0-5]|2[0-4][0-9]|[0-1]?[0-9]{1,2}))";
    static const std::string ip = one_num + "."s + one_num + "."s + one_num + "."s + one_num;
    static const std::regex expr(ip);

    return std::regex_match(txt, expr);
}

std::string get_correct_ip() {
    std::string res;

    std::cout << "Enter SERVER IP: $";
    std::getline(std::cin, res);

    while(!is_correct_ip(res)) {
        std::cout << "IP adress \"" << res << "\" is incorrect. Enter correct SERVER IP: $";
        std::getline(std::cin, res);
    }

    return res;
}

void as_client() {
    static const int port = 3333;
    static const size_t max_buffer_size = 65001;

    try {
        Recorder recorder(ma_format_u8, 1);
        boost::asio::io_context io_context;
        udp::socket socket(io_context, udp::v4());
        boost::system::error_code ec;

        while (1) {
            std::string server_ip = get_correct_ip();

            auto rec_result = recorder.Record(65000, 1.5s);
            std::cout << "Record done!\n";

            int size = std::min(max_buffer_size, rec_result.frames * recorder.GetFrameSize());

            auto endpoint = udp::endpoint(net::ip::make_address(server_ip, ec), port);
            socket.send_to(net::buffer(rec_result.data.data(), size), endpoint);
            std::cout << "Sended\n";
        }

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

}

void as_server() {
    static const int port = 3333;
    static const size_t max_buffer_size = 65001;
    Player player(ma_format_u8, 1);

    try {
        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        std::array<char, max_buffer_size> buf;

        while (4 != 5) {
            udp::endpoint ep;
            auto size = socket.receive_from(boost::asio::buffer(buf), ep);
            std::cout << "Recive record\n";

            size = size / player.GetFrameSize();

            player.PlayBuffer(buf.data(), size, 1.5s);
            std::cout << "Played\n";
        }

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

int audio(int argc, char** argv) {
    Recorder recorder(ma_format_u8, 1);
    Player player(ma_format_u8, 1);

    while (true) {
        std::string str;

        std::cout << "Press Enter to record message..." << std::endl;
        std::getline(std::cin, str);

        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording done" << std::endl;

        player.PlayBuffer(rec_result.data.data(), rec_result.frames, 1.5s);
        std::cout << "Playing done" << std::endl;
    }

    return 0;
}

int server() {
    static const int port = 3333;
    static const size_t max_buffer_size = 1024;

    try {
        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        // Запускаем сервер в цикле, чтобы можно было работать со многими клиентами
        for (;;) {
            // Создаём буфер достаточного размера, чтобы вместить датаграмму.
            std::array<char, max_buffer_size> recv_buf;
            udp::endpoint remote_endpoint;

            // Получаем не только данные, но и endpoint клиента
            auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

            std::cout << "Client said "sv << std::string_view(recv_buf.data(), size) << std::endl;

            // Отправляем ответ на полученный endpoint, игнорируя ошибку.
            // На этот раз не отправляем перевод строки: размер датаграммы будет получен автоматически.
            boost::system::error_code ignored_error;
            socket.send_to(boost::asio::buffer("Hello from UDP-server"sv), remote_endpoint, 0, ignored_error);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

int client(int argc, const char** argv) {
    static const int port = 3333;
    static const size_t max_buffer_size = 1024;

    if (argc != 2) {
        std::cout << "Usage: "sv << argv[0] << " <server IP>"sv << std::endl;
        return 1;
    }

    try {
        net::io_context io_context;

        // Перед отправкой данных нужно открыть сокет.
        // При открытии указываем протокол (IPv4 или IPv6) вместо endpoint.
        udp::socket socket(io_context, udp::v4());

        boost::system::error_code ec;
        auto endpoint = udp::endpoint(net::ip::make_address(argv[1], ec), port);
        socket.send_to(net::buffer("Hello from UDP-client"sv), endpoint);

        // Получаем данные и endpoint.
        std::array<char, max_buffer_size> recv_buf;
        udp::endpoint sender_endpoint;
        size_t size = socket.receive_from(net::buffer(recv_buf), sender_endpoint);

        std::cout << "Server responded "sv << std::string_view(recv_buf.data(), size) << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
