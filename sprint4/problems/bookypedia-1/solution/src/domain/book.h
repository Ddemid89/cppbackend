#pragma once
#include <string>

#include "../util/tagged_uuid.h"
#include "author.h"

namespace domain {

namespace detail {
struct BookTag {};
}  // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

class Book {
public:
    Book(BookId id, AuthorId auth_id, std::string title, int year)
        : id_(std::move(id))
        , auth_id_(auth_id)
        , title_(std::move(title))
        , year_(year){
    }

    const BookId& GetBookId() const noexcept {
        return id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return auth_id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    const int GetYear() const noexcept {
        return year_;
    }

private:
    BookId id_;
    AuthorId auth_id_;
    std::string title_;
    int year_;
};

class BookRepository {
public:
    virtual void Save(const ui::detail::AddBookParams&, BookId) = 0;
    virtual std::vector<ui::detail::BookInfo> GetBooks() = 0;
    virtual std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& name) = 0;
protected:
    ~BookRepository() = default;
};

}  // namespace domain
