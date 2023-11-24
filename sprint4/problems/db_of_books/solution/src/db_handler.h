#pragma once

#include <vector>

#include <pqxx/pqxx>

#include "domain.h"

namespace db_handler {

using namespace std::literals;
// libpqxx использует zero-terminated символьные литералы вроде "abc"_zv;
using pqxx::operator"" _zv;

class DbHandler {
public:
    DbHandler(std::string& db_url) : conn_{db_url} {
        pqxx::work w(conn_);

        w.exec(
            "CREATE TABLE IF NOT EXISTS books(id SERIAL, title varchar(100) NOT NULL, author varchar(100) NOT NULL, year integer NOT NULL, ISBN char(13) UNIQUE);"
        );
        w.commit();
    }

    bool AddBook(domain::Book& book) {
        pqxx::work w(conn_);
        std::string query = "INSERT INTO books VALUES (DEFAULT, "s;
        query += w.quote(w.esc(book.title)) + ", "s;
        query += w.quote(w.esc(book.author)) + ", "s;
        query += std::to_string(book.year) + ", "s;

        if (book.ISBN != "") {
            query += w.quote(book.ISBN);
        } else {
            query += "NULL"s;
        }

        query += ");"s;

        try {
            w.exec(query);
            w.commit();
            return true;
        }  catch (...) {
            return false;
        }
    }

    std::vector<domain::Book> AllBooks() {
        pqxx::read_transaction r(conn_);

        std::string query = "SELECT * FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC;";
        auto resp = r.query<int, std::string, std::string, int, std::optional<std::string>>(query);

        std::vector<domain::Book> result;

        for (const auto& [id, title, author, year, ISBN] : resp) {
            domain::Book book;

            book.title = title;
            book.author = author;
            book.year = year;
            book.id = id;
            if (ISBN.has_value()) {
                book.ISBN = *ISBN;
            }

            result.push_back(book);
        }

        return result;
    }

private:
    pqxx::connection conn_;
};

} // namespace db_handler
