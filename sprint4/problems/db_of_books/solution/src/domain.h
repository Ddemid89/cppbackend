#pragma once
#include <string>
#include <optional>
#include <stdexcept>

namespace domain {
    struct Book {
        int id;
        std::string title;
        std::string author;
        int year;
        std::string ISBN;
    };

    enum class Action {
        AddBook,
        AllBooks,
        Exit
    };

    struct Query {
        Action action;
        std::optional<Book> book;
    };

    Action GetActionByStr(const std::string& str) {
        if (str == "add_book") {
            return Action::AddBook;
        } else if (str == "all_books") {
            return Action::AllBooks;
        } else if (str == "exit") {
            return Action::Exit;
        }

        throw std::invalid_argument("Unsupported action str");
    }

} // namespace domain
