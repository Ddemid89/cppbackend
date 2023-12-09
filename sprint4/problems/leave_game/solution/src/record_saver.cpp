#include "record_saver.h"

namespace record_saver_pq {

RecordSaverPQ::RecordSaverPQ(const std::string& db_url)
    : pool_{std::thread::hardware_concurrency(),
            [&db_url](){ return std::make_shared<pqxx::connection>(db_url); } }
{
    auto check_table = R"(
            CREATE TABLE IF NOT EXISTS retired_players (
                id UUID CONSTRAINT record_id_constraint PRIMARY KEY,
                name varchar(100),
                score integer NOT NULL,
                play_time_ms integer NOT NULL
            )
        )"_zv;

    auto check_index = R"(
            CREATE INDEX IF NOT EXISTS retired_players_idx ON retired_players (score DESC, play_time_ms, name)
        )"_zv;

    auto conn = pool_.GetConnection();
    pqxx::work work{*conn};

    work.exec(check_table);
    work.exec(check_index);

    work.commit();
}

void RecordSaverPQ::Save(std::vector<game_manager::Retiree>&& retirees) {
    auto conn = pool_.GetConnection();
    pqxx::work work{*conn};

    for (const auto& ret : retirees) {
        AddRetiree(work, ret);
    }

    work.commit();
}

std::vector<game_manager::Retiree> RecordSaverPQ::GetRecords(std::size_t start, std::size_t max_size) {
    auto conn = pool_.GetConnection();
    pqxx::read_transaction tr{*conn};

    auto query =
            "SELECT name, score, play_time_ms\
            FROM retired_players\
            ORDER BY score DESC, play_time_ms, name\
            LIMIT " + std::to_string(max_size) +
          " OFFSET " + std::to_string(start) + ";";

    std::vector<game_manager::Retiree> result;

    auto response = tr.query<std::string, size_t, size_t>(query);

    for (auto& [name, score, time] : response) {
        game_manager::Retiree ret;
        ret.name = name;
        ret.score = score;
        ret.game_time = time;
        result.push_back(ret);
    }

    return result;
}

void RecordSaverPQ::AddRetiree(pqxx::work& work, const game_manager::Retiree& ret) {
    work.exec_params(R"(
            INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4);)"_zv,
                     to_string(NewUUID()), ret.name, ret.score, ret.game_time
            );
}

UUIDType RecordSaverPQ::NewUUID() {
    return boost::uuids::random_generator()();
}

} // namespace record_saver_pq
