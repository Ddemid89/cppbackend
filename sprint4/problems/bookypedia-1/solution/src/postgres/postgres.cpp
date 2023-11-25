#include "postgres.h"

#include <pqxx/zview.hxx>
#include <boost/uuid/uuid.hpp>
#include <pqxx/pqxx>
#include "../ui/view.h"

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), work.esc(author.GetName()));
    work.commit();
}

std::vector<ui::detail::AuthorInfo> AuthorRepositoryImpl::Get() {
    pqxx::read_transaction tr(connection_);

    std::string query = "SELECT * FROM authors ORDER BY name;";

    auto resp = tr.query<std::string, std::string>(query);

    std::vector<ui::detail::AuthorInfo> result;

    for (auto& [id, name] : resp) {
        ui::detail::AuthorInfo auth{id, name};
        result.push_back(auth);
    }

    return result;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT books_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL,
    title varchar(100) UNIQUE NOT NULL,
    publication_year integer NOT NULL
);
)"_zv);

    // коммитим изменения
    work.commit();
}

void BookRepositoryImpl::Save(const ui::detail::AddBookParams& params, domain::BookId book_id) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
    INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4);
    )"_zv,
        book_id.ToString(), params.author_id, params.title, params.publication_year);
    work.commit();
}

std::vector<ui::detail::BookInfo> BookRepositoryImpl::GetBooks() {
    pqxx::read_transaction tr(connection_);

    std::string query = "SELECT publication_year, title FROM books ORDER BY title;";

    auto resp = tr.query<int, std::string>(query);

    std::vector<ui::detail::BookInfo> result;

    for (auto& [year, title] : resp) {
        ui::detail::BookInfo book;
        book.title = title;
        book.publication_year = year;
        result.push_back(book);
    }

    return result;
}

std::vector<ui::detail::BookInfo> BookRepositoryImpl::GetAuthorBooks(const std::string& author_id) {
    pqxx::read_transaction tr(connection_);

    std::string query
        = "SELECT publication_year, title FROM books WHERE author_id='" + author_id + "' ORDER BY publication_year;";

    auto resp = tr.query<int, std::string>(query);

    std::vector<ui::detail::BookInfo> result;

    for (auto& [year, title] : resp) {
        ui::detail::BookInfo book;
        book.title = title;
        book.publication_year = year;
        result.push_back(book);
    }

    return result;
}

}  // namespace postgres
