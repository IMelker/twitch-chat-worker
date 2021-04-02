//
// Created by imelker on 06.03.2021.
//

#include "Options.h"

Options::Options(std::string program, std::string help)
    : options(std::move(program), std::move(help)) {
    this->options.allow_unrecognised_options();
}

Options::~Options() = default;


void Options::parse(int argc, const char *const *argv) {
    this->res = options.parse(argc, argv);
}

bool Options::checkOption(const std::string &key) {
    return this->res.count(key);
}
