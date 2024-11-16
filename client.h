#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>

#include <QString>
#include <QThread>

class Client : public QThread {
public:
  Client();
  void recv();
  void resp();
  ~Client();
  void run() override;
  void request_stop();
  void listen();

private:
  int fd;
  int port{12345};
  bool should_stop{false};
  bool should_listen{true};
  struct sockaddr_in server_addr, client_addr;
  int buffer_size{1024};
  // QString buffer;
  socklen_t addr_len{sizeof(server_addr)};
  char buffer[1024]{};
};

#endif // CLIENT_H
