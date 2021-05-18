//
// Created by imelker on 01.03.2021.
//

#include <iostream>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "Utils.h"

namespace Utils::String
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

bool toBool(std::string_view str) {
    return str == "t";
}

}

namespace Utils::UUIDv4
{

uint128_t from_uint8_16_array(uint8_t *data) {
    return { htobe64(*((uint64_t *)(data))), htobe64(*((uint64_t *)(data + 8))) };
}

uint128_t uint128() {
    static auto gen = boost::uuids::random_generator();
    auto uuid = gen();
    return from_uint8_16_array(uuid.data);
}

std::string string() {
    static auto gen = boost::uuids::random_generator();
    return boost::uuids::to_string(gen());
}

std::pair<uint128_t, std::string> pair() {
    static auto gen = boost::uuids::random_generator();
    auto uuid = gen();
    return {from_uint8_16_array(uuid.data), boost::uuids::to_string(uuid)};
}

}

