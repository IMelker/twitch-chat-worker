//
// Created by l2pic on 25.04.2021.
//

#include <thread>
#include <chrono>

#include "IRCConnectionConfig.h"
#include "IRCClientConfig.h"

#include "IRCClient.h"
#include "IRCSelectorPool.h"

#include "../common/SysSignal.h"


int main() {
    IRCSelectorPool pool;
    pool.init(1);

    IRCConnectionConfig conConfig;
    conConfig.host = "irc.chat.twitch.tv";
    conConfig.port = 6667;
    conConfig.connect_attemps_limit = 20;

    IRCClientConfig cliConfig1;
    cliConfig1.user = "projackrus";
    cliConfig1.nick = "ProjackRus";
    cliConfig1.password = "oauth:keo5bg9w1djrgnbkr6kri37fchs43w";
    cliConfig1.id = 1;
    cliConfig1.channels_limit = 20;
    cliConfig1.auth_per_sec_limit = 2;
    cliConfig1.command_per_sec_limit = 1;
    cliConfig1.whisper_per_sec_limit = 3;

    IRCClient client(conConfig, cliConfig1, &pool);

    /*IRCClientConfig cliConfig2;
    cliConfig2.user = "bservice";
    cliConfig2.nick = "bservice";
    cliConfig2.password = "oauth:cpbhzl20hr8795iechk2oy5ku8m347";
    cliConfig2.id = 2;
    cliConfig2.channels_limit = 20;
    cliConfig2.auth_per_sec_limit = 2;
    cliConfig2.command_per_sec_limit = 1;
    cliConfig2.whisper_per_sec_limit = 3;
    IRCClient client2(conConfig, cliConfig2, &pool);*/

    while(!SysSignal::serviceTerminated()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return 0;
}

