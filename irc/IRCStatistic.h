//
// Created by l2pic on 27.04.2021.
//

#ifndef IRCTEST__IRCSTATISTIC_H_
#define IRCTEST__IRCSTATISTIC_H_

#include <atomic>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

#define DUMP_VAR_TO(arg, res) res.append(#arg " = ").append(std::to_string(relx_get(arg))).append(", ")

namespace {
template<typename T> auto relx_get(const std::atomic<T> &var) { return var.load(std::memory_order_relaxed); }
template<typename T, typename U> void relx_set(std::atomic<T> &var, const U& other) { var.store(other, std::memory_order_relaxed); }
template<typename T, typename U> void relx_add(std::atomic<T> &var, const U& other) { atomic_fetch_add_explicit(&var, other, std::memory_order_relaxed); }
template<typename T, typename U> void relx_sub(std::atomic<T> &var, const U& other) { atomic_fetch_sub_explicit(&var, other, std::memory_order_relaxed); }
template<typename T> void relx_inc(std::atomic<T> &var) { relx_add(var, 1); }
template<typename T> void relx_dec(std::atomic<T> &var) { relx_sub(var, 1); }
#define relx_easy_add(rhs, field) relx_add(field, relx_get(rhs.field))
}

class IRCStatistic
{
  public:
    friend json statsToJson(const IRCStatistic& stats);

  public:
    std::string dump() {
        std::string res;
        DUMP_VAR_TO(connects.updated, res);
        DUMP_VAR_TO(connects.success, res);
        DUMP_VAR_TO(connects.failed, res);
        DUMP_VAR_TO(connects.loggedin, res);
        DUMP_VAR_TO(connects.disconnected, res);
        DUMP_VAR_TO(channels.count, res);
        DUMP_VAR_TO(commands.in.bytes, res);
        DUMP_VAR_TO(commands.in.count, res);
        DUMP_VAR_TO(commands.out.bytes, res);
        DUMP_VAR_TO(commands.out.count, res);
        return res;
    }

    void clear() {
        relx_set(connects.updated, 0);
        relx_set(connects.success, 0);
        relx_set(connects.failed, 0);
        relx_set(connects.loggedin, 0);
        relx_set(connects.disconnected, 0);
        relx_set(channels.count, 0);
        relx_set(commands.in.bytes, 0);
        relx_set(commands.in.count, 0);
        relx_set(commands.out.bytes, 0);
        relx_set(commands.out.count, 0);
    }

    void connectsUpdatedSet(unsigned long long time) { relx_set(connects.updated, time); }
    void connectsSuccessInc() { relx_inc(connects.success); }
    void connectsFailedInc() { relx_inc(connects.failed); }
    void connectsLoggedInInc() { relx_inc(connects.loggedin); }
    void connectsDisconnectedInc() { relx_inc(connects.disconnected); }

    void channelsCountInc() { relx_inc(channels.count); }
    void channelsCountDec() { relx_dec(channels.count); }
    void channelsCountSet(int count) { relx_set(channels.count, count); }

    void commandsInBytesAdd(unsigned long long count) { relx_add(commands.in.bytes, count); }
    void commandsInCountInc() { relx_inc(commands.in.count); }
    void commandsOutBytesAdd(unsigned long long count) { relx_add(commands.out.bytes, count); }
    void commandsOutCountInc() { relx_inc(commands.out.count); }

    inline IRCStatistic& operator+(const IRCStatistic& rhs) noexcept {
        relx_easy_add(rhs, connects.updated);
        relx_easy_add(rhs, connects.success);
        relx_easy_add(rhs, connects.failed);
        relx_easy_add(rhs, connects.loggedin);
        relx_easy_add(rhs, connects.disconnected);
        relx_easy_add(rhs, channels.count);
        relx_easy_add(rhs, commands.in.bytes);
        relx_easy_add(rhs, commands.in.count);
        relx_easy_add(rhs, commands.out.bytes);
        relx_easy_add(rhs, commands.out.count);
        return *this;
    }
    inline IRCStatistic& operator+=(const IRCStatistic & rhs) {
        return operator+(rhs);
    }

  private:
    struct {
        std::atomic_ullong updated = 0;
        std::atomic_int success = 0;
        std::atomic_int failed = 0;
        std::atomic_int loggedin = 0;
        std::atomic_int disconnected = 0;
    } connects;
    struct {
        std::atomic_int count = 0;
    } channels;
    struct {
        struct {
            std::atomic_ullong bytes = 0;
            std::atomic_int count = 0;
        } in;
        struct {
            std::atomic_ullong bytes = 0;
            std::atomic_int count = 0;
        } out;
    } commands;
};

inline json statsToJson(const IRCStatistic& stats) {
    json res = json::object();
    res["connects"] = {
        {"updated", relx_get(stats.connects.updated)},
        {"success", relx_get(stats.connects.success)},
        {"failed", relx_get(stats.connects.failed)},
        {"loggedin", relx_get(stats.connects.loggedin)},
        {"disconnected", relx_get(stats.connects.disconnected)}
    };
    res["channels"] = {{"count", relx_get(stats.channels.count)}};
    res["commands"] = {
        { "in", {
            {"bytes", relx_get(stats.commands.in.bytes)},
            {"count", relx_get(stats.commands.in.count)}
        }
        },
        { "out", {
            {"bytes", relx_get(stats.commands.out.bytes)},
            {"count", relx_get(stats.commands.out.count)}
        }
        }
    };
    return res;
};

class IRCStatisticProvider {
  public:
    ~IRCStatisticProvider() = default;

    [[nodiscard]] virtual const IRCStatistic& statistic() {
        return stats;
    };

  protected:
    IRCStatistic stats;
};

#endif //IRCTEST__IRCSTATISTIC_H_
