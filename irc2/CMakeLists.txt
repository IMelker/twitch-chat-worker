cmake_minimum_required(VERSION 3.15)
project(irctest)

# -Wpedantic disabled because of https://github.com/abseil/abseil-cpp/issues/157
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

include_directories(../common)

add_executable(irctest
        main.cpp
        IRCClient.h IRCClient.cpp
        IRCSessionContext.h
        IRCSession.h IRCSession.cpp
        IRCSessionCallback.h IRCSessionCallback.cpp
        IRCSelector.h IRCWorker.cpp
        IRCSelectorPool.h IRCSelectorPool.cpp
        ../common/SysSignal.h ../common/SysSignal.cpp
        ../common/Clock.h)
target_link_libraries(irctest pthread ircclient)
