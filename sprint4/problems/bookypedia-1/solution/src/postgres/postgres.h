#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include <vector>

#include "../domain/author.h"
#include "../domain/book.h"

#include "../ui/view.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author) override;

    std::vector<ui::detail::AuthorInfo> Get() override;
private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {

    }

    void Save(const ui::detail::AddBookParams& params, domain::BookId) override;

    std::vector<ui::detail::BookInfo> GetBooks() override;

    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& name) override;

private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks() & {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl     books_{connection_};
};

}  // namespace postgres
