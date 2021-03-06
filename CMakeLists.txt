cmake_minimum_required(VERSION 3.15)
project(chatbot)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

add_executable(chatbot
        main.cpp
        common/Utils.h common/Utils.cpp
        ircclient/IRCClient.h ircclient/IRCClient.cpp
        ircclient/IRCHandler.h ircclient/IRCHandler.cpp
        ircclient/IRCSocket.h ircclient/IRCSocket.cpp
        )

target_link_libraries(chatbot pthread)
