cmake_minimum_required(VERSION 3.11)

project(book_manager CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable (
    book_manager
    src/main.cpp
    src/json_handler.h
    src/db_handler.h
    src/domain.h
    src/book_manager.h
)

target_link_libraries(book_manager PRIVATE CONAN_PKG::libpqxx CONAN_PKG::boost)
