message("[main]")

cmake_minimum_required(VERSION 3.15)
project(chatcontroller)

# -Wpedantic disabled because of https://github.com/abseil/abseil-cpp/issues/157
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

if (USE_ASAN)
    message("- [ASAN]")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -static-libasan")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -static-libasan")
elseif(USE_LSAN)
    message("- [LSAN]")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak -static-liblsan")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak -static-liblsan")
elseif(USE_TSAN)
    message("- [TSAN]")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -static-libtsan")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -static-libtsan")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

# app info
set(APP_NAME "TLuaBot")
set(APP_BIN_NAME "chatcontroller")
set(APP_VERSION_MAJOR 0)
set(APP_VERSION_MINOR 1)
set(APP_VERSION_PATCH 0)
set(APP_VERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH})
set(APP_AUTHOR_EMAIL "twitch.luabot@gmail.com")
set(APP_PID_PATH "/run/chatcontroller.pid")
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

# apt-get install libpcre++-dev
message("- [pcrecpp]")
find_library(PCRE_LIBRARY pcrecpp REQUIRED)

message("- [lua]")
find_package(Lua 5.3 EXACT REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

add_subdirectory(http)

configure_file(${CMAKE_SOURCE_DIR}/config/app.h.in ${CMAKE_BINARY_DIR}/app.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/config/config.toml.in ${CMAKE_BINARY_DIR}/config.toml @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/config/start.sh.in ${CMAKE_BINARY_DIR}/start.sh @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/config/stop.sh.in ${CMAKE_BINARY_DIR}/stop.sh @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/config/app.service.in ${CMAKE_BINARY_DIR}/${APP_BIN_NAME}.service @ONLY)

include_directories(${CMAKE_BINARY_DIR})
include_directories(common)

set(SOURCES
        main.cpp
        HttpController.h HttpController.cpp
        HttpNotifier.h HttpNotifier.cpp
        Controller.h Controller.cpp
        DBController.h DBController.cpp
        MessageProcessor.h MessageProcessor.cpp
        Storage.h Storage.cpp
        StatsCollector.cpp StatsCollector.h
        ChatMessage.h
        So5Helpers.h)

set(COMMON_SOURCES
        common/network/Socket.h common/network/Socket.cpp
        common/network/UDPSocket.h common/network/UDPSocket.cpp
        common/network/TCPSocket.h common/network/TCPSocket.cpp
        common/network/TCPListener.h common/network/TCPListener.cpp
        common/network/Selector.h common/network/Selector.cpp
        common/SysSignal.h common/SysSignal.cpp
        common/Options.h common/Options.cpp
        common/Config.h common/Config.cpp
        common/ThreadPool.h common/ThreadPool.cpp
        common/ThreadName.h
        common/BufferStatic.h
        common/Logger.h common/Logger.cpp common/LoggerFactory.h
        common/Utils.h common/Utils.cpp
        common/Timer.h common/Timer.cpp
        common/Clock.h
        common/ScopeExec.h
        common/URI.cpp common/URI.h
        common/Exception.h  common/Exception.cpp)

set(DB_SOURCES
        db/DBConnectionLock.h
        db/DBConnection.h db/DBConnection.cpp
        db/DBConnectionPool.h db/DBConnectionPool.cpp
        db/pg/PGConnection.h db/pg/PGConnection.cpp
        db/pg/PGConnectionPool.h db/pg/PGConnectionPool.cpp
        db/ch/CHConnection.h db/ch/CHConnection.cpp
        db/ch/CHConnectionPool.h db/ch/CHConnectionPool.cpp)

set(IRC_SOURCES
        irc/IRCClient.h irc/IRCClient.cpp
        irc/IRCSessionInterface.h irc/IRCSession.h irc/IRCSession.cpp
        irc/IRCSessionContext.h
        irc/IRCChannelList.cpp irc/IRCChannelList.h
        irc/IRCSessionCallback.h irc/IRCSessionCallback.cpp
        irc/IRCSelectorPool.h irc/IRCSelectorPool.cpp
        irc/IRCSelector.h irc/IRCSelector.cpp
        irc/IRCController.h irc/IRCController.cpp
        irc/IRCMessage.h
        irc/IRCSessionListener.h
        irc/IRCSessionInterface.h)

set(BOT_SOURCES
        bot/BotsEnvironment.h bot/BotsEnvironment.cpp
        bot/BotEngine.h bot/BotEngine.cpp
        bot/BotConfiguration.h
        bot/BotEvents.h
        bot/events/BotEvent.h bot/handlers/BotEventHandler.h
        bot/events/BotTimerEvent.h
        bot/events/BotRequestEvent.h
        bot/events/BotMessageEvent.h
        bot/handlers/BotMessageEventHandler.h
        bot/handlers/BotMessageEventHandlerCommand.h bot/handlers/BotMessageEventHandlerCommand.cpp
        bot/handlers/BotMessageEventHandlerLua.h bot/handlers/BotMessageEventHandlerLua.cpp)

# executable configuration
message("\n[executable]")
add_executable(${APP_BIN_NAME} ${SOURCES} ${COMMON_SOURCES} ${DB_SOURCES} ${IRC_SOURCES} ${BOT_SOURCES})

add_subdirectory(common/tests)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-fstack-protector-all)
endif()

target_link_libraries(${APP_BIN_NAME}
        stdc++fs
        pthread
        ssl
        crypto
        ircclient
        sobjectizer::StaticLib
        httpserver httpclient ${Boost_LIBRARIES}
        nlohmann_json
        fmt
        absl::strings
        spdlog
        clickhouse-cpp-lib-static
        ${PostgreSQL_LIBRARIES}
        langdetectpp
        pcrecpp
        sol2::sol2 ${LUA_LIBRARIES})
