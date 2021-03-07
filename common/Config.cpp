//
// Created by l2pic on 06.03.2021.
//

#include "Config.h"

#include <fstream> //required for toml::parse_file()
#include <sstream>

#define DEBUG_CONFIG
#ifdef DEBUG_CONFIG
#define PRINT(f, ...) printf((f"\n"), ##__VA_ARGS__)
#else
#define PRINT()
#endif

Config::Config(const std::string &filepath) {
    PRINT("Config: \"%s\"", filepath.c_str());
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
        PRINT("Failed to parse: \"%s\"", error.what());
    }
}

void Config::save() {
    saveAs(this->path);
}

void Config::saveAs(const std::string &filepath) {
    std::ofstream out(filepath);
    out << this->data;
    out.close();

    PRINT("Config saved to \"%s\"", this->path.c_str());
}

std::string Config::dump() {
    std::stringstream str;
    str << toml::json_formatter{this->data};
    return str.str();
}
