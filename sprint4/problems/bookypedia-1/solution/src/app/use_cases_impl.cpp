#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<ui::detail::AuthorInfo> UseCasesImpl::GetAuthors() {
    return authors_.Get();
}

void UseCasesImpl::AddBook(const ui::detail::AddBookParams& params) {
    return books_.Save(params, BookId::New());
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetBooks() {
    return books_.GetBooks();
}

std::vector<ui::detail::BookInfo> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    return books_.GetAuthorBooks(author_id);
}

}  // namespace app
