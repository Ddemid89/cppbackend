#include "book_manager.h"
#include "iostream"

int main(int argc, const char** argv) {
    if (argc == 1) {
        std::cout << "Usage: book_manager <db_url>";
        return EXIT_SUCCESS;
    } else if (argc != 2) {
        std::cout << "Invalid command line";
        return EXIT_FAILURE;
    }
    book_manager::BookManager bm(std::cin, std::cout, argv[1]);

    bm.Run();
}
