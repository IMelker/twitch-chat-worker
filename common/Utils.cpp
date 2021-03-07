//
// Created by imelker on 01.03.2021.
//

#include "Utils.h"

namespace Utils
{

namespace String
{

void toUpper(std::string *str) {
    std::transform(str->begin(), str->end(), str->begin(), ::toupper);
}

std::string toUpper(std::string_view str) {
    std::string upperStr(str);
    toUpper(&upperStr);
    return upperStr;
}

void toLower(std::string *str) {
    std::transform(str->begin(), str->end(), str->begin(), ::tolower);
}

std::string toLower(std::string_view str) {
    std::string upperStr(str);
    toLower(&upperStr);
    return upperStr;
}

}

}


