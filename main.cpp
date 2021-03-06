#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <thread>
#include <csignal>

#include "ircclient/IRCClient.h"

volatile bool running;

void signalHandler(int signal) { running = false; }

class ConsoleCommandHandler
{
  public:
    bool AddCommand(std::string name, int argCount,
                    void (*handler)(const std::string& params,
                                    IRCClient * client)) {
        CommandEntry entry{};
        entry.argCount = argCount;
        entry.handler = handler;
        std::transform(name.begin(), name.end(), name.begin(), towlower);
        _commands.insert(std::pair<std::string, CommandEntry>(name, entry));
        return true;
    }

    void ParseCommand(std::string command, IRCClient *client) {
        if (_commands.empty()) {
            std::cout << "No commands available." << std::endl;
            return;
        }

        if (command[0] == '/')
            command = command.substr(1); // Remove the slash

        std::string name = command.substr(0, command.find(' '));
        std::string args = command.substr(command.find(' ') + 1);
        int argCount = std::count(args.begin(), args.end(), ' ');

        std::transform(name.begin(), name.end(), name.begin(), towlower);

        auto itr = _commands.find(name);
        if (itr == _commands.end()) {
            std::cout << "Command not found." << std::endl;
            return;
        }

        if (++argCount < itr->second.argCount) {
            std::cout << "Insuficient arguments." << std::endl;
            return;
        }

        (*(itr->second.handler))(args, client);
    }

  private:
    struct CommandEntry
    {
        int argCount;
        void (*handler)(const std::string& arguments, IRCClient *client);
    };

    std::map<std::string, CommandEntry> _commands;
};

ConsoleCommandHandler commandHandler;

void msgCommand(const std::string& arguments, IRCClient *client) {
    std::string to = arguments.substr(0, arguments.find(' '));
    std::string text = arguments.substr(arguments.find(' ') + 1);

    client->sendIRC("PRIVMSG #" + to + " :" + text);
};

void joinCommand(const std::string& channel, IRCClient *client) {
    client->sendIRC("JOIN #" + channel);
}

void partCommand(const std::string& channel, IRCClient *client) {
    client->sendIRC("PART #" + channel);
}

void joinFreak(const std::string& channel, IRCClient *client) {
    client->sendIRC("JOIN #dmitry_lixxx");
    client->sendIRC("JOIN #mokrivskyi");
    client->sendIRC("JOIN #zloyn");
    client->sendIRC("JOIN #ahrinyan");
    client->sendIRC("JOIN #yuuechka");
    client->sendIRC("JOIN #strogo");
    client->sendIRC("JOIN #exileshow");
    client->sendIRC("JOIN #karavay46");
    client->sendIRC("JOIN #gensyxa");
    client->sendIRC("JOIN #buster");
    client->sendIRC("JOIN #quickhuntik");
    client->sendIRC("JOIN #mapke");
    client->sendIRC("JOIN #kostbi4");
    client->sendIRC("JOIN #by_owl");
    client->sendIRC("JOIN #des0ut");
    client->sendIRC("JOIN #zark");
    client->sendIRC("JOIN #pch3lk1n");
    client->sendIRC("JOIN #biest1x");
    client->sendIRC("JOIN #chr1swave");
    client->sendIRC("JOIN #guacamolemolly");
}

void inputThread(IRCClient *client) {
    std::string command;

    commandHandler.AddCommand("msg", 2, &msgCommand);
    commandHandler.AddCommand("join", 1, &joinCommand);
    commandHandler.AddCommand("part", 1, &partCommand);
    commandHandler.AddCommand("freak", 1, &joinFreak);

    while (true) {
        getline(std::cin, command);
        if (command.empty()) {
            client->sendIRC("ping");
            continue;
        }

        if (command[0] == '/')
            commandHandler.ParseCommand(command, client);
        else
            client->sendIRC(command);

        if (command == "quit")
            break;
    }
}

int main(int argc, char *argv[]) {
    //if (argc < 3) {
    //    std::cout << "Insuficient parameters: host port [nick] [user]" << std::endl;
    //    return 1;
    //}

    const char *host = "irc.chat.twitch.tv"; //argv[1];
    int port = 6667;//std::atoi(argv[2]);
    std::string nick("projackrus");
    std::string user("ProjackRus");
    std::string password("oauth:vgw4e7swtyaw57c8rh1jc1rg7log0t");

    //if (argc >= 4)
    //    nick = argv[3];
    //if (argc >= 5)
    //    user = argv[4];

    // IRC Client
    IRCClient client(true);

    std::thread thread{&inputThread, &client};

    //std::cout << "Socket initialized. Connecting..." << std::endl;
    if (client.connect((char *)host, port)) {
        //std::cout << "connected. Loggin in..." << std::endl;

        if (client.login(nick, user, password)) {
            //std::cout << "Logged." << std::endl;

            running = true;
            signal(SIGINT, signalHandler);

            while (client.connected() && running)
                client.receive();
        }

        if (client.connected())
            client.disconnect();

        //std::cout << "Disconnected." << std::endl;
    }

    if (thread.joinable())
        thread.join();
}
