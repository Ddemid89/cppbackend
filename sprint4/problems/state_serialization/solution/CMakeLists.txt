cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(common_sources STATIC
        src/loot_generator.cpp
        src/game_manager.cpp
        src/move_manager.cpp
        src/collision_detector.cpp
)

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
        src/api_handler.h
        src/api_handler.cpp
        src/body_types.h
        src/body_types.cpp
        src/resp_maker.h
        src/resp_maker.cpp
        src/model_serialization.h
        src/model_serialization.cpp
        src/move_manager.h
        src/ticker.h
        src/http_strs.h
        src/loot_generator.h
        src/collision_detector.h
        src/geom.h
        src/game_serialization.h
)

add_executable(game_server_tests
        tests/loot_generator_tests.cpp
        tests/model_tests.cpp
        tests/collision-detector-tests.cpp
        tests/serialization-tests.cpp
        src/loot_generator.h
        src/game_manager.h
        src/model.h
        src/model.cpp
        src/move_manager.h
        src/collision_detector.h        
        src/ticker.h
        src/tagged.h
        src/game_serialization.h
)

include(CTest)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        include(${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake) 
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        include(${CONAN_BUILD_DIRS_CATCH2_RELEASE}/Catch.cmake)
endif()

catch_discover_tests(game_server_tests)

target_include_directories(common_sources PUBLIC CONAN_PKG::boost)
target_link_libraries(common_sources PUBLIC Threads::Threads CONAN_PKG::boost)

target_link_libraries(game_server PRIVATE common_sources)

target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2)
target_link_libraries(game_server_tests PRIVATE common_sources)
