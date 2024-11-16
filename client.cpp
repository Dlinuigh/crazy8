#include "client.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

Client::Client() {
  // 创建 UDP 套接字
  if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("Socket creation failed");
  }
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    qDebug("fcntl failed to get socket flags");
    return;
  }
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  // char interface[] = "wlp4s0";
  // if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, interface,
  //                strlen(interface)) < 0) {
  //   perror("setsockopt SO_BINDTODEVICE");
  //   close(fd);
  // }
  memset(&server_addr, 0, addr_len);
  memset(&client_addr, 0, addr_len);

  // 配置服务器地址
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  // server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(port);
  client_addr.sin_addr.s_addr = htons(INADDR_ANY);
  if (bind(fd, (const struct sockaddr *)&client_addr, addr_len) < 0) {
    perror("Bind failed");
  }
  qDebug("Socket Create Success");
}
void Client::recv() {
  ssize_t len = recvfrom(fd, buffer, buffer_size, MSG_WAITALL, NULL, NULL);
  buffer[len] = '\0';
}
void Client::resp() {
  const char *message = "Hello, Server!";
  sendto(fd, message, strlen(message), MSG_CONFIRM,
         (const struct sockaddr *)&server_addr, addr_len);
}
Client::~Client() {
  qDebug("Server Will Stop in most 1s");
  close(fd);
  if (not should_stop) {
    request_stop();
    sleep(1);
  }
}
void Client::run() {
  while (not should_stop) {
    if (should_listen) {
      listen();
    }
    sleep(1);
  }
}
void Client::request_stop() {
  should_stop = true;
  qDebug("Stop Client");
}
void Client::listen() {
  ssize_t recv_len = recvfrom(fd, buffer, sizeof(buffer), 0,
                              (struct sockaddr *)&server_addr, &addr_len);
  if (recv_len < 0) {
    if (errno == EAGAIN) {

    } else {
      perror("Error in Recv BoardCast");
      qDebug("%d", errno);
    }
  } else {
    buffer[recv_len] = '\0';
    server_addr.sin_addr.s_addr = inet_addr(buffer);
    qDebug("Recv BoardCast Message: %s", buffer);
    resp();
    should_listen = false;
  }
}
