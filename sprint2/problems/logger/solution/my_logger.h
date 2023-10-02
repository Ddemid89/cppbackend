#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
public:
    auto GetTime() const {
        if (manual_ts_) {
            std::lock_guard lg(manual_ts_mutex_);
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::gmtime(&t_c), "%F %T");
    }

    std::ofstream GetCurrentFileName() {
        return std::ofstream{GetFileTimeStamp(), std::ios::app};
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);

        char date[] = "YYYY_MM_DD";
        size_t max_size = std::size(date);
        std::strftime(date, max_size, "%Y_%m_%d", std::gmtime(&t_c));

        return "/var/log/sample_log_"s + date + ".log"s;
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard lg(stream_mutex_);
        auto out = GetCurrentFileName();
        out << GetTimeStamp() << ": ";
        ((out << args), ...);
        out << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard lg(manual_ts_mutex_);
        manual_ts_ = ts;
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    mutable std::mutex manual_ts_mutex_;
    std::mutex stream_mutex_;
};
