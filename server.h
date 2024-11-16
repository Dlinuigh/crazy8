#ifndef SERVER_H
#define SERVER_H
#include <QString>
#include <QThread>
#include <netdb.h>
#include <netinet/in.h>
// #define SERVER_IP "127.0.0.1"
class Server : public QThread {
  Q_OBJECT
public:
  Server();
  void radiate();
  void run() override;
  ~Server();
  void request_stop();
  void boardcast();
  void recv();
  void resp();

private:
  int fd;
  bool should_stop{false};
  bool should_boardcast{true};
  struct sockaddr_in server_addr, client_addr, boardcast_addr;
  int port{12345};
  int buffer_size{1024};
  // QString buffer{};
  char buffer[1024];
  char host[NI_MAXHOST];
  socklen_t client_len{sizeof(client_addr)};
  void get_ip();
};

#endif // SERVER_H
