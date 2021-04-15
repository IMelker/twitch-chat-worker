message("[main]")

cmake_minimum_required(VERSION 3.15)
project(chatcontroller)

# -Wpedantic disabled because of https://github.com/abseil/abseil-cpp/issues/157
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

# app info
set(APP_NAME "TLuaChat")
set(APP_VERSION_MAJOR 0)
set(APP_VERSION_MINOR 0)
set(APP_VERSION_PATCH 1)
set(APP_VERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH})
set(APP_AUTHOR_EMAIL "l2pickn@yandex.ru")
execute_process(COMMAND git rev-parse --verify HEAD OUTPUT_VARIABLE APP_GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git log -1 --pretty=format:%ad HEAD OUTPUT_VARIABLE APP_GIT_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
message("-- App: ${APP_NAME}.${APP_VERSION}.${APP_GIT_HASH} ${APP_GIT_DATE}")

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message("-- Build: ${CMAKE_BUILD_TYPE}")

# third-party libraries
message("\n[third-party]")
add_subdirectory(third-party)
include_directories(third-party/spdlog/include)
include_directories(third-party/toml)
include_directories(third-party/fmt/include)
include_directories(third-party/opts/include)
include_directories(third-party/clickhouse)
include_directories(third-party/langdetect/langdetectpp/include)
include_directories(third-party/sol/include)

message("- [pg]")
find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIRS})

message("- [lua]")
find_package(Lua 5.3 EXACT REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

add_subdirectory(http)

configure_file(${CMAKE_SOURCE_DIR}/config/app.h.in ${CMAKE_BINARY_DIR}/app.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/config/config.toml.in ${CMAKE_BINARY_DIR}/config.toml @ONLY)
include_directories(${CMAKE_BINARY_DIR})


# executable configuration
message("\n[executable]")
add_executable(chatcontroller
        main.cpp
        HttpController.h HttpController.cpp
        Controller.h Controller.cpp
        DBController.h DBController.cpp
        ChatMessage.h
        MessageProcessor.h MessageProcessor.cpp
        Storage.h Storage.cpp
        common/SysSignal.h common/SysSignal.cpp
        common/Options.h common/Options.cpp
        common/Config.h common/Config.cpp
        common/ThreadPool.h common/ThreadPool.cpp
        common/BufferStatic.h
        common/Logger.h common/Logger.cpp common/LoggerFactory.h
        common/Utils.h common/Utils.cpp
        common/Timer.h common/Timer.cpp
        common/Clock.h
        db/DBConnectionLock.h
        db/DBConnection.h db/DBConnection.cpp
        db/DBConnectionPool.h db/DBConnectionPool.cpp
        db/pg/PGConnection.h db/pg/PGConnection.cpp
        db/pg/PGConnectionPool.h db/pg/PGConnectionPool.cpp
        db/ch/CHConnection.h db/ch/CHConnection.cpp
        db/ch/CHConnectionPool.h db/ch/CHConnectionPool.cpp
        irc/IRCSocket.h irc/IRCSocket.cpp
        irc/IRCClient.h irc/IRCClient.cpp
        irc/IRCHandler.h irc/IRCHandler.cpp
        irc/IRCWorker.h irc/IRCWorker.cpp
        irc/IRCWorkerPool.h irc/IRCWorkerPool.cpp
        bot/BotsEnvironment.h bot/BotsEnvironment.cpp
        bot/BotEngine.h bot/BotEngine.cpp
        bot/BotConfiguration.h bot/BotLogger.h
        )

add_subdirectory(common/tests)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-fstack-protector-all)
endif()

target_link_libraries(chatcontroller
        stdc++fs
        pthread ssl crypto
        sobjectizer::StaticLib
        httpserver
        nlohmann_json
        spdlog
        clickhouse-cpp-lib-static
        ${PostgreSQL_LIBRARIES}
        absl::strings
        langdetectpp
        sol2::sol2 ${LUA_LIBRARIES})
