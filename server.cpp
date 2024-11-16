#include "server.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <unistd.h>
Server::Server() {
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    qDebug("Socket Creation Failed!");
  }
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    qDebug("fcntl failed to get socket flags");
    return;
  }
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  // char interface[] = "virbr0";
  // if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, interface,
  //                strlen(interface)) < 0) {
  //   perror("setsockopt SO_BINDTODEVICE");
  //   close(fd);
  // }
  int broadcast = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) <
      0) {
    perror("setsockopt failed");
    close(fd);
  }
  memset(&server_addr, 0, sizeof(server_addr));
  memset(&client_addr, 0, sizeof(client_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  // 配置服务器地址
  boardcast_addr.sin_family = AF_INET;
  boardcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255"); // 监听所有接口
  boardcast_addr.sin_port = htons(port);                         // 设置端口

  // 绑定套接字
  if (bind(fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Bind failed");
  }
  // FIXME 终于问题理清了，当程序在虚拟环境下运行会被分配一个合理的ip
  get_ip();
  server_addr.sin_addr.s_addr = inet_addr(host);
  qDebug("Socket Bind Success!");
  qDebug("Host IP Addr is %s", host);
}
void Server::radiate() {
  while (not should_stop) {
    if (should_boardcast) {
      boardcast();
    }
    recv();
    sleep(1);
  }
}
void Server::recv() {
  ssize_t len = recvfrom(fd, buffer, buffer_size, 0,
                         (struct sockaddr *)&client_addr, &client_len);
  if (len < 0) {
    if (errno == EAGAIN) {

    } else {
      perror("recv error");
    }
  } else {
    if (memcmp(&server_addr, &client_addr, client_len) != 0) {
      buffer[len] = '\0';
      qDebug("Message From Client: %s", buffer);
      should_boardcast = false;
    }
  }
}
void Server::resp() {
  const char *response = "Message received";
  sendto(fd, response, strlen(response), MSG_CONFIRM,
         (const struct sockaddr *)&client_addr, client_len);
}
void Server::boardcast() {
  sendto(fd, host, strlen(host), MSG_CONFIRM,
         (const struct sockaddr *)&boardcast_addr, client_len);
}
Server::~Server() {
  qDebug("Server Will Stop in most 1s");
  close(fd);
  if (not should_stop) {
    request_stop();
    sleep(1);
  }
}
void Server::run() { radiate(); }
void Server::request_stop() {
  should_stop = true;
  qDebug("Recv Signal End");
}
void Server::get_ip() {
  struct ifaddrs *ifaddr, *ifa;
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
  }
  for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
                                               // 获取接口的IP地址
      getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST,
                  nullptr, 0, NI_NUMERICHOST);
      qDebug("%s", host);
    }
  }
  freeifaddrs(ifaddr);
}
