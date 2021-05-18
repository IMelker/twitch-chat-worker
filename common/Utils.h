//
// Created by imelker on 01.03.2021.
//

#ifndef CHATBOT_COMMON_UTILS_H_
#define CHATBOT_COMMON_UTILS_H_

#include <cassert>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <array>
#include <charconv>

using uint128_t = std::pair<uint64_t, uint64_t>;

// helper type for stream visitor
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace Utils::String
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

bool toBool(std::string_view str);

namespace Number {

inline void appendHex(std::string& str, unsigned value, int width) {
    assert(width > 0 && width < 64);
    char buffer[64];
    std::sprintf(buffer, "%0*X", width, value);
    str.append(buffer);
}

inline std::string formatHex(unsigned value, int width)
{
    std::string result;
    appendHex(result, value, width);
    return result;
}

}

}

namespace Utils::UUIDv4
{
    uint128_t uint128();
    std::string string();
    std::pair<uint128_t, std::string> pair();
}

#endif //CHATBOT_COMMON_UTILS_H_
