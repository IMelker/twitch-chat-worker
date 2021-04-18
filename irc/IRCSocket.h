#ifndef _IRCSOCKET_H
#define _IRCSOCKET_H

#define SOCKET int

#include <string>

class IRCSocket
{
  public:
    IRCSocket();
    ~IRCSocket();

    bool connect(char const *host, int port);
    void disconnect();

    [[nodiscard]] bool connected() const { return this->established; };

    bool send(char const *data, size_t len) const;
    int receive(char *buf, int maxSize);

  private:
    void create();

    SOCKET sock = -1;
    bool established = false;
};

#endif
