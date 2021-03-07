//
// Created by l2pic on 06.03.2021.
//

#ifndef CHATSNIFFER_COMMON_OPTIONS_H_
#define CHATSNIFFER_COMMON_OPTIONS_H_

#include <cxxopts.hpp>

class Options
{
  public:
    explicit Options(std::string program, std::string help = "");
    ~Options();

    void parse(int argc, const char* const* argv);

    template<typename T>
    void addOption(const std::string& opt, const std::string& shortOpt, const std::string& desc,
                   const std::string& value = "", const std::string& group = "", const std::string& help = "") {
        this->options.add_option(group, shortOpt, opt, desc, cxxopts::value<T>()->default_value(value), help);
    }

    template<typename T>
    const T& getValue(const std::string& key) {
        return this->res[key].as<T>();
    }

    bool checkOption(const std::string& key);
  private:
    cxxopts::Options options;
    cxxopts::ParseResult res;
};

#endif //CHATSNIFFER_COMMON_OPTIONS_H_
