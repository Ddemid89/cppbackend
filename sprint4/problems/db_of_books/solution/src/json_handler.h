#pragma once

#include <boost/json.hpp>
#include "domain.h"

namespace domain {
namespace json = boost::json;

Book tag_invoke(json::value_to_tag<Book>&, const json::value& book) {
    Book result;
    result.author = book.at("author").as_string();
    result.title  = book.at("title").as_string();

    if (book.at("ISBN").is_string()) {
        result.ISBN = book.at("ISBN").as_string();
    }

    result.year = book.at("year").as_int64();
    return result;
}

void tag_invoke(json::value_from_tag, json::value& jv, const Book& book) {
    jv = {
        {"author", book.author}
       ,{"title",  book.title}
       ,{"year",   book.year}
       ,{"id",     book.id}
    };

    if (book.ISBN == "") {
        jv.as_object().emplace("ISBN", json::value{});
    } else {
        jv.as_object().emplace("ISBN", book.ISBN);
    }
}

} // namespace domain

namespace json_h {

namespace json = boost::json;

std::string SerializeBooks(const std::vector<domain::Book>& books) {
    json::value jv = json::value_from(books);
    return json::serialize(jv);
}

domain::Query ParseQuery (const std::string& query) {
    json::value jv;

    try {
        jv = json::parse(query);
    }  catch (...) {
        throw std::runtime_error("Incomplit JSON: \"" + query + "\".");
    }

    auto act = jv.at("action").as_string();
    std::string act_str(act.data(), act.size());

    domain::Query result;

    result.action = domain::GetActionByStr(act_str);

    if (result.action == domain::Action::AddBook) {
        result.book = json::value_to<domain::Book>(jv.at("payload"));
    }

    return result;
}

} // namespace json_h
