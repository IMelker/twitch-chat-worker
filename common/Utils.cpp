//
// Created by imelker on 01.03.2021.
//

#include "Utils.h"

std::vector<std::string> split(std::string const &text, char sep) {
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos) {
        tokens.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;
}
