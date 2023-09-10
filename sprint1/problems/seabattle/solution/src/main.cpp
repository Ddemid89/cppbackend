//#ifdef WIN32
//#include <sdkddkver.h>
//#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        PrintFields();
        if (my_initiative) {
            MyTurn(socket);
        }

        while(1) {
            OtherTurn(socket);
            if (my_field_.IsLoser()) {
                std::cout << "You lose!\n";
                return;
            }
            MyTurn(socket);
            if (other_field_.IsLoser()) {
                std::cout << "You WIN!!!!!!!!\n";
                return;
            }
        }

    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 >= 8) return std::nullopt;
        if (p2 < 0 || p2 >= 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        std::string res(static_cast<char>(move.first) + 'A', 1);
        res.push_back(static_cast<char>(move.second) + '1');
        return res;
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    std::string GetTurn() {
        std::cout << "\n\n\nYour turn: ";
        std::string txt;
        std::cin >> txt;

        auto res = ParseMove(txt);

        while(!res) {
            std::cout << "Incorrect field name([A-H][1-8]). Your turn: ";
            std::cin >> txt;
            res = ParseMove(txt);
        }

        return txt;
    }

    void MyTurn(tcp::socket& socket) {
        while (1) {
            auto turn = GetTurn();

            if(!WriteExact(socket, turn)) {
                throw std::runtime_error("Error sending data");
            }

            auto ans = ReadExact<1>(socket);

            if (!ans) {
                throw std::runtime_error("Error reading data");
            }

            char res = (*ans)[0] - '0';

            auto [y, x] = *ParseMove(turn);

            if (res == 0) {
                other_field_.MarkMiss(x, y);
                std::cout << "Miss\n";
                PrintFields();
                return;
            } else if (res == 1) {
                other_field_.MarkHit(x, y);
                std::cout << "Hit!!!\n";
            } else if (res == 2) {
                other_field_.MarkKill(x, y);
                if (other_field_.IsLoser()) {
                    return;
                }
                std::cout << "Kill!!!!!!!!!!\n";
            } else {
                throw std::logic_error("MyTurn: something wrong!");
            }
            PrintFields();
        }
    }

    void OtherTurn(tcp::socket& socket) {
        //Возможно, нужно самому сообщить, что корабль убит
        boost::system::error_code ec;

        while(1) {
            std::cout << "\n\n\nWaiting for turn..." << std::endl;
            auto turn = ReadExact<2>(socket);

            if (!turn) {
                throw std::runtime_error("Can`t read data!");
            }

            std::cout << "Turn: " << *turn << std::endl;

            auto coors = ParseMove(*turn);

            auto res = my_field_.Shoot(coors->second, coors->first );

            WriteExact(socket, std::to_string(static_cast<int>(res)));

            PrintFields();

            if (res == SeabattleField::ShotResult::MISS) {
                return;
            } else if (res == SeabattleField::ShotResult::KILL
                       && my_field_.IsLoser()) {
                return;
            }

        }
    }

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);
    net::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..."sv << std::endl;

    boost::system::error_code ec;
    tcp::socket socket{io_context};
    acceptor.accept(socket, ec);

    if (ec) {
        throw std::runtime_error("Can't accept connection");
    }

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);

    if (ec) {
        throw std::logic_error("Wrong IP format");
    }

    net::io_context io_context;
    tcp::socket socket{io_context};
    socket.connect(endpoint, ec);

    if (ec) {
        throw std::runtime_error("Can't connect to server");
    }

    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
