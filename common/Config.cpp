//
// Created by imelker on 06.03.2021.
//

#include "Config.h"

#include <fstream> //required for toml::parse_file()
#include <sstream>

Config::Config(const std::string &filepath) {
    open(filepath);
}

Config::Config() = default;
Config::~Config() = default;

void Config::open(const std::string &filepath) {
    this->path = filepath;
    reopen();
}

void Config::reopen() {
    try {
        this->data = toml::parse_file(this->path);
    } catch (const toml::parse_error& error) {
        fprintf(stderr, "Failed to parse: \"%s\"", error.what());
    }
}

void Config::save() {
    saveAs(this->path);
}

void Config::saveAs(const std::string &filepath) {
    std::ofstream out(filepath);
    out << this->data;
    out.close();
}

std::string Config::dump() {
    std::stringstream str;
    str << toml::json_formatter{this->data};
    return str.str();
}
