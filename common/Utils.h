//
// Created by imelker on 01.03.2021.
//

#ifndef CHATBOT_COMMON_UTILS_H_
#define CHATBOT_COMMON_UTILS_H_

#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <charconv>

namespace Utils
{

namespace String
{

void toUpper(std::string *str);
std::string toUpper(std::string_view str);

void toLower(std::string *str);
std::string toLower(std::string_view str);

template<typename T = int> T toNumber(std::string_view str) {
    static_assert(std::is_integral_v<T>, "std::from_chars now supported only for integral values");
    T res = 0;
    std::from_chars(str.data(), str.data() + str.size(), res);
    return res;
}

}

}

#endif //CHATBOT_COMMON_UTILS_H_
