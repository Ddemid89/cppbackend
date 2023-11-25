#pragma once

#include <string>
#include <vector>

namespace ui {
namespace detail {
struct AuthorInfo;
struct BookInfo;
struct AddBookParams;
}
}

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual std::vector<ui::detail::AuthorInfo> GetAuthors() = 0;
    virtual void AddBook(const ui::detail::AddBookParams& params) = 0;
    virtual std::vector<ui::detail::BookInfo> GetBooks() = 0;
    virtual std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& name) = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app
