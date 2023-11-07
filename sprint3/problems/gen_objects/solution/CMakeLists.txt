cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
        src/main.cpp
        src/http_server.cpp
        src/http_server.h
        src/sdk.h
        src/model.h
        src/model.cpp
        src/tagged.h
        src/boost_json.cpp
        src/json_loader.h
        src/json_loader.cpp
        src/request_handler.cpp
        src/request_handler.h
        src/json_keys.h
        src/logging_request_handler.h
        src/logger.h
        src/logger.cpp
        src/game_manager.h
        src/game_manager.cpp
        src/api_handler.h
        src/api_handler.cpp
        src/body_types.h
        src/body_types.cpp
        src/resp_maker.h
        src/resp_maker.cpp
        src/model_serialization.h
        src/model_serialization.cpp
        src/move_manager.h
        src/move_manager.cpp
        src/ticker.h
        src/http_strs.h
        src/loot_generator.cpp
        src/loot_generator.h
)

add_executable(catch_tests
        tests/loot_generator_tests.cpp
        src/loot_generator.cpp
        src/loot_generator.h)

target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost)
target_link_libraries(catch_tests PRIVATE CONAN_PKG::catch2)