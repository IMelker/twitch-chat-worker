//
// Created by imelker on 01.03.2021.
//

#ifndef CHATBOT_COMMON_UTILS_H_
#define CHATBOT_COMMON_UTILS_H_

#include <vector>
#include <string>
#include <string_view>
#include <algorithm>

namespace Utils
{

namespace String
{

void toUpper(std::string *str);
std::string toUpper(std::string_view str);

void toLower(std::string *str);
std::string toLower(std::string_view str);

}

}

#endif //CHATBOT_COMMON_UTILS_H_
