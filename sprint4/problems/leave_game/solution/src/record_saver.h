#pragma once

#include <pqxx/pqxx>

#include <pqxx/zview.hxx>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>
#include <pqxx/connection>
#include <pqxx/transaction>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <thread>

#include "game_manager.h"
#include "connection_pool.h"

namespace record_saver_pq {

using namespace std::literals;
using pqxx::operator"" _zv;
using UUIDType = boost::uuids::uuid;

class RecordSaverPQ : public game_manager::RecordSaverInterface {
public:
    RecordSaverPQ(const std::string& db_url);

    void Save(std::vector<game_manager::Retiree>&& retirees) override;

    std::vector<game_manager::Retiree> GetRecords(std::size_t start, std::size_t max_size) override;

private:

    void AddRetiree(pqxx::work& work, const game_manager::Retiree& ret);

    UUIDType NewUUID();

    connection_pool::ConnectionPool pool_;
};

} // namespace record_saver_pq
