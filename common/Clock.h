//
// Created by imelker on 23.03.2021.
//

#ifndef CHATSNIFFER_COMMON_CLOCK_H_
#define CHATSNIFFER_COMMON_CLOCK_H_

#include <chrono>
#include <iomanip>

using namespace std::chrono;

template <typename ClockT>
class CurrentTime
{
    template<struct tm* represent(const time_t *)> static std::string utc() {
        auto timer = ClockT::to_time_t(ClockT::now());
        std::tm bt = *represent(&timer);
        std::ostringstream oss;
        oss << std::put_time(&bt, "%F %T");
        return oss.str();
    }
    template<struct tm* represent(const time_t *)> static std::string utcWithMilli() {
        auto now = ClockT::now();
        auto ms = duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        auto timer = ClockT::to_time_t(now);
        std::tm bt = *represent(&timer);
        std::ostringstream oss;
        oss << std::put_time(&bt, "%F %T");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
  public:
    static long long seconds() {
        return ClockT::now().time_since_epoch() / ::seconds(1);
    }
    static long long milliseconds() {
        return ClockT::now().time_since_epoch() / ::milliseconds(1);
    }
    static long long microseconds() {
        return ClockT::now().time_since_epoch() / ::microseconds(1);
    }
    static long long nanoseconds() {
        return ClockT::now().time_since_epoch() / ::nanoseconds(1);
    }
    static std::string utcTime() { return CurrentTime::utc<&std::time>(); }
    static std::string utcTimeWithMilli() { return CurrentTime::utcWithMilli<&std::time>(); }
    static std::string utcLocalTime() { return CurrentTime::utc<&std::localtime>(); }
    static std::string utcLocalTimeWithMilli() { return CurrentTime::utcWithMilli<&std::localtime>(); }
    static std::string utcGMTime() { return CurrentTime::utc<&std::gmtime>(); }
    static std::string utcGMTimeWithMilli() { return CurrentTime::utcWithMilli<&std::gmtime>(); }
};

#endif //CHATSNIFFER_COMMON_CLOCK_H_
