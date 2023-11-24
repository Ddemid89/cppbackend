#pragma once

#include <iostream>
#include <vector>

#include "domain.h"
#include "json_handler.h"
#include "db_handler.h"

#include <fstream>

namespace book_manager {

class BookManager {
public:
    BookManager(std::istream& in, std::ostream& out, std::string db_name)
        : in_(in), out_(out), db_h_(db_name) {}

    void Run() {

        for(;;) {
            domain::Query query = ReadQuery();

            switch (query.action) {
            case domain::Action::AddBook:
                AddBookAndReport(*query.book);
                break;
            case domain::Action::AllBooks:
                GetAndPrintAllBooks();
                break;
            case domain::Action::Exit:
                return;
            }
        }
    }

private:

    void AddBookAndReport(domain::Book& book) {
        bool res = db_h_.AddBook(book);
        out_ << "{\"result\":" << std::boolalpha << res << "}" << std::endl;
    }

    void GetAndPrintAllBooks() {
        std::vector<domain::Book> books{db_h_.AllBooks()};
        out_ << json_h::SerializeBooks(books) << std::endl;
    }

    domain::Query ReadQuery() {
        std::string query;
        std::getline(in_, query);
        return json_h::ParseQuery(query);
    }

    std::istream& in_;
    std::ostream& out_;
    db_handler::DbHandler db_h_;
};


} // namespace book_manager
