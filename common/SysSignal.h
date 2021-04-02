//
// Created by imelker on 06.03.2021.
//

#ifndef CHATBOT_COMMON_SYSSIGNAL_H_
#define CHATBOT_COMMON_SYSSIGNAL_H_

#include <atomic>

class SysSignal {
  public:
    static void setupSignalHandling();
    static void signalHandler(int signal_number);
    static bool serviceTerminated();
    static void setServiceTerminated(bool state);

  private:
    static std::atomic<bool> terminated;
};

#endif //CHATBOT_COMMON_SYSSIGNAL_H_
