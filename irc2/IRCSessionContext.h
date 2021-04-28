//
// Created by l2pic on 25.04.2021.
//

#ifndef IRCTEST__IRCSESSIONCONTEXT_H_
#define IRCTEST__IRCSESSIONCONTEXT_H_

class IRCSessionCallback;
class IRCClient;
struct IRCSessionContext {
    IRCSessionCallback *callback = nullptr;
    IRCClient *parent = nullptr;
};

#endif //IRCTEST__IRCSESSIONCONTEXT_H_
