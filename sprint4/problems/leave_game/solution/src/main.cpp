#include <boost/program_options.hpp>
#include "sdk.h"

#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>

#include <iostream>
#include <thread>
#include <filesystem>

#include "json_loader.h"
#include "request_handler.h"
#include "logging_request_handler.h"
#include "logger.h"
#include "game_manager.h"
#include "game_serialization.h"
#include "record_saver.h"

using namespace std::literals;

struct Args {
    std::string file;
    std::string dir;
    std::string state_file;
    uint64_t save_period = 0;
    uint64_t milliseconds = 0;
    bool random_spawn = false;
};

namespace po = boost::program_options;

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;
    po::options_description desc{"Allowed options"s};

    Args args;
    desc.add_options()
    ("help,h", "produce help message")
    ("tick-period,t", po::value(&args.milliseconds)->value_name("milliseconds"s), "set tick period")
    ("config-file,c", po::value(&args.file)->value_name("file"s), "set config file path")
    ("www-root,w", po::value(&args.dir)->value_name("dir"s), "set static files root")
    ("state-file", po::value(&args.state_file)->value_name("state_file"s), "set state file")
    ("save-state-period", po::value(&args.save_period)->value_name("save_period"s), "set save period")
    ("randomize-spawn-points", "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << desc;
        return std::nullopt;
    }

    if(!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file have not been specified"s);
    }

    if(!vm.contains(("www-root"s))) {
        throw std::runtime_error("Static files root have not been specified"s);
    }

    args.random_spawn = vm.contains("randomize-spawn-points");

    return args;
}

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
using tcp = net::ip::tcp;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    try {
        auto args = ParseCommandLine(argc, argv);
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(args->file);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        const char* db_url = std::getenv("GAME_DB_URL");

        if (!db_url) {
            throw std::runtime_error("No database url. Variable GAME_DB_URL not defined.");
        }

        // 2.1. Инициализируем endpoint
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port {8080};

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        const std::filesystem::path static_path(std::filesystem::weakly_canonical(args->dir));

        game_manager::GameManager game_m{game, ioc, args->random_spawn, args->milliseconds};

        game_m.SetRecordSaver(std::make_shared<record_saver_pq::RecordSaverPQ>(db_url));

        http_handler::RequestHandler handler{game_m, static_path};

        auto l_handler = logging_handler::MakeHandler(handler);

        json_logger::JsonLogger& logger = json_logger::JsonLogger::GetInstance();

        std::shared_ptr<game_serialization::Serializator> serializator;

        if (args->state_file != "") {
            serializator = std::make_shared<game_serialization::Serializator>(
                        game_m, args->state_file, args->save_period
            );

            serializator->Load();

            if (args->save_period != 0) {
                game_m.Subscribe(serializator);
            }
        }

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        http_server::ServeHttp(ioc, {address, port}, [&l_handler](auto&& req, auto&& send) {
            l_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        logger.LogServerStarted({address, port});

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        serializator->Save();

        logger.LogServerNormalFinish();

    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        json_logger::JsonLogger::GetInstance().LogServerErrorFinish(ex);
        return EXIT_FAILURE;
    }
}
