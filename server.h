#ifndef SERVER_H
#define SERVER_H
#include <QString>
#include <QThread>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "crazy8.h"

class Server final : public QThread {
    Q_OBJECT
public:
    int max_players{0};
    // TODO: 1. server is also a player 2. server is not a player
    // TODO: for 1, host start game and join game, client get the board info, and reply.
    // TODO: for 2, every player get board info from server and reply to server.
    explicit Server(int _max_players) : max_players(_max_players) {
        try {
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (fd == -1) {
                throw std::runtime_error("Failed to create socket");
            }
            fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
            constexpr int broadcast{true};
            if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) {
                throw std::runtime_error("Failed to set socket options");
            }
            memset(&addr, 0, addr_len);
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;
            memset(&broadcast_addr, 0, addr_len);
            broadcast_addr.sin_family = AF_INET;
            broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;
            broadcast_addr.sin_port = htons(port);
            if (bind(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) < 0) {
                throw std::runtime_error("Failed to bind");
            }
            // FIXME 终于问题理清了，当程序在虚拟环境下运行会被分配一个合理的ip
            set_addr();
            buffer.resize(buffer_size);
            buffer.clear();
        } catch (std::runtime_error &e) {
            qDebug("Error: %s", e.what());
        }
    }

    void radiate() {
        while (not should_stop) {
            if (should_broadcast) {
                broadcast();
            }
            receive();
            sleep(1);
        }
    }
    void run() override { radiate(); }
    ~Server() override {
        qDebug("Server Will Stop in 1s");
        close(fd);
        if (not should_stop) {
            request_stop();
            sleep(1);
        }
    }
    void request_stop() {
        should_stop = true;
        qDebug("Receive Signal End");
    }
    void broadcast() const {
        char message[]{"Hello Players!"};
        sendto(fd, message, strlen(message), MSG_CONFIRM, reinterpret_cast<const sockaddr *>(&broadcast_addr),
               addr_len);
    }
    void receive() {
        // TODO: receive info, and handle.
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        try {
            if (recvfrom(fd, buffer.data(), buffer_size, 0, reinterpret_cast<sockaddr *>(&client_addr), &len) == -1) {
                if (errno == EAGAIN) {
                    // not an error. just ignore.
                } else {
                    throw std::runtime_error("Failed to receive message");
                }
            } else {
                if (should_broadcast) {
                    if (memcmp(&addr, &client_addr, addr_len) != 0) {
                        players_addr.push_back(client_addr);
                        buffer.push_back(QChar('\0'));
                        qDebug("Message From Client: %p", buffer.data());
                    }
                    if (players_addr.size() == max_players) {
                        should_broadcast = false;
                    }
                }
            }
        } catch (std::runtime_error &e) {
            qDebug("Error: %s", e.what());
        }
    }
    void response(QString &data, const MessageType type) {
        for (auto _addr: players_addr) {
            Message message{};
            message.type = type;
            memcpy(message.data, data.data(), sizeof(message.data));
            constexpr int len = sizeof(message);
            char buffer[len];
            memcpy(buffer, &message, len);
            try {
                if(sendto(fd, buffer, len, 0, reinterpret_cast<const sockaddr *>(&_addr), addr_len)==-1) {
                    throw std::runtime_error("Failed to send message");
                }
            } catch (std::runtime_error &e) {
                qDebug("Error: %s", e.what());
            }
        }
    }

private:
    int fd{};
    bool should_stop{false};
    bool should_broadcast{true};
    sockaddr_in addr{}, broadcast_addr{};
    uint16_t port{12345};
    QString buffer{};
    int buffer_size{1024};
    std::vector<sockaddr_in> players_addr{};
    socklen_t addr_len{sizeof(addr)};
    void set_addr() {
        char host[NI_MAXHOST]{};
        ifaddrs *ifaddr;
        try {
            if (getifaddrs(&ifaddr) == -1) {
                throw std::runtime_error("getifaddrs() failed");
            }
        } catch (std::runtime_error &e) {
            qDebug("Error: %s", e.what());
        }
        for (const ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr->sa_family == AF_INET) {
                getnameinfo(ifa->ifa_addr, addr_len, host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            }
        }
        freeifaddrs(ifaddr);
        addr.sin_addr.s_addr = inet_addr(host);
    }
};
#endif // SERVER_H
