//
// Created by l2pic on 06.03.2021.
//

#ifndef CHATSNIFFER_COMMON_CONFIG_H_
#define CHATSNIFFER_COMMON_CONFIG_H_

#include <string_view>
#include <toml.hpp>

class Config
{
  public:
    Config();
    explicit Config(const std::string& filepath);
    ~Config();

    template <typename KeyType, typename ValueType>
    auto insert(KeyType&& key, ValueType&& val) {
        return this->data.insert_or_assign(key, val);
    }

    template <typename KeyType>
    auto operator[](KeyType&& key) {
        return this->data[key];
    }

    void open(const std::string& filepath);
    void reopen();
    void save();
    void saveAs(const std::string& filepath);

    std::string dump();

  private:
    std::string path{};
    toml::table data{};
};

#endif //CHATSNIFFER_COMMON_CONFIG_H_
