//
// Created by l2pic on 15.05.2021.
//

#ifndef CHATCONTROLLER_COMMON_SCOPEEXEC_H_
#define CHATCONTROLLER_COMMON_SCOPEEXEC_H_

#ifndef SCOPEEXEC_H_
#define SCOPEEXEC_H_

#include <type_traits>

template <typename OnCreate, typename OnExit, class Enable = void>
class ScopeExec {
    using OnCreateResultType = typename std::result_of_t<OnCreate(void)>;
  public:
    explicit ScopeExec(OnCreate onCreate, OnExit onExit)
      : onCreate(onCreate), onExit(onExit) {
        rc = onCreate();
    };
    ~ScopeExec() { reset(); }

    auto creteResult() const { return rc; };

    void reset() {
        if (!exitEmitted) {
            exitEmitted = true;
            onExit();
        }
    }
  private:
    OnCreate onCreate;
    OnExit onExit;
    OnCreateResultType rc;
    bool exitEmitted = false;
};

template <typename OnCreate, typename OnExit>
class ScopeExec<OnCreate, OnExit, typename std::enable_if_t<std::is_void_v<std::result_of_t<OnCreate(void)>>>> {
  public:
    ScopeExec(OnCreate onCreate, OnExit onExit) : onCreate(onCreate), onExit(onExit) { onCreate(); }
    ~ScopeExec() { reset(); }
    void reset() {
        if (!exitEmitted) {
            exitEmitted = true;
            onExit();
        }
    }
  private:
    OnCreate onCreate;
    OnExit onExit;
    bool exitEmitted = false;
};

#endif // SCOPEEXEC_H_


#endif //CHATCONTROLLER_COMMON_SCOPEEXEC_H_
