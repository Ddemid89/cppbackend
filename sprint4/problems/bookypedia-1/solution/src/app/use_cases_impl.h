#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors}, books_{books} {
    }

    void AddAuthor(const std::string& name) override;

    std::vector<ui::detail::AuthorInfo> GetAuthors() override;

    void AddBook(const ui::detail::AddBookParams& params) override;

    std::vector<ui::detail::BookInfo> GetBooks() override;

    std::vector<ui::detail::BookInfo> GetAuthorBooks(const std::string& name) override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app
