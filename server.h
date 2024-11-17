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
    QVector<in_addr> players_addr{};
    // TODO: 1. server is also a player 2. server is not a player
    // TODO: for 1, host start game and join game, client get the board info, and reply.
    // TODO: for 2, every player get board info from server and reply to server.
    Server()=default;
    explicit Server(int _max_players) : max_players(_max_players) {
        try {
            if((fd = socket(AF_INET, SOCK_DGRAM, 0))==-1){
                throw errno;
            }
            fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
            constexpr int broadcast{true};
            if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) {
                throw errno;
            }
            memset(&addr, 0, addr_len);
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;
            memset(&broadcast_addr, 0, addr_len);
            broadcast_addr.sin_family = AF_INET;
            broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;
            broadcast_addr.sin_port = htons(port);
            if (bind(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == -1) {
                throw errno;
            }
            set_addr();
            qDebug("Server started at %s", inet_ntoa(addr.sin_addr));
        } catch (...) {
            qDebug("Error: %s", strerror(errno));
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
        // TODO for safe reason, I will introduce a cypher method.
        char data[]{"Hello Players!"};
        crazy8::Message message{};
        message.type = crazy8::MessageType::broadcast;
        memcpy(message.data, data, sizeof(data));
        sendto(fd, &message, sizeof(crazy8::Message), MSG_CONFIRM, reinterpret_cast<const sockaddr *>(&broadcast_addr),
               addr_len);
    }
    void receive() {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        crazy8::Message message{};
        try {
            if (recvfrom(fd, &message, sizeof(crazy8::Message), 0, reinterpret_cast<sockaddr *>(&client_addr), &len) == -1) {
                if (errno == EAGAIN) {
                    // not an error. just ignore.
                } else {
                    throw errno;
                }
            } else {
                if (should_broadcast) {
                    if (addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                        using namespace crazy8;
                        switch (message.type) {
                            case crazy8::MessageType::reply_broadcast: {
                                players_addr.push_back(client_addr.sin_addr);
                                emit PlayerJoin();
                                qDebug("Player %s Joined", inet_ntoa(client_addr.sin_addr));
                                break;
                            }
                            case MessageType::play: {
                                break;
                            }
                            case MessageType::uncover: {
                                break;
                            }
                            case MessageType::disconnect: {
                                break;
                            }
                            default: ;
                        }
                    }
                    if (players_addr.size() == max_players) {
                        should_broadcast = false;
                    }
                }
            }
        } catch (...) {
                qDebug("Error: %s", strerror(errno));
        }
    }
    void response(QString &data, const crazy8::MessageType type) {
        for (auto _addr: players_addr) {
            crazy8::Message message{};
            message.type = type;
            memcpy(message.data, data.data(), sizeof(message.data));
            constexpr int len = sizeof(message);
            char buffer[len];
            memcpy(buffer, &message, len);
            try {
                if(sendto(fd, buffer, len, 0, reinterpret_cast<const sockaddr *>(&_addr), addr_len)==-1) {
                    throw errno;
                }
            } catch (...) {
                qDebug("Error: %s", strerror(errno));
            }
        }
    }

private:
    int fd{};
    bool should_stop{false};
    bool should_broadcast{true};
    sockaddr_in addr{}, broadcast_addr{};
    uint16_t port{12345};
    socklen_t addr_len{sizeof(addr)};
    void set_addr() {
        char host[NI_MAXHOST]{};
        ifaddrs *ifaddr;
        try {
            if (getifaddrs(&ifaddr) == -1) {
                throw errno;
            }
        } catch (...) {
            qDebug("Error: %s", strerror(errno));
        }
        for (const ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr->sa_family == AF_INET) {
                getnameinfo(ifa->ifa_addr, addr_len, host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            }
        }
        freeifaddrs(ifaddr);
        addr.sin_addr.s_addr = inet_addr(host);
    }
signals:
    void PlayerJoin();
};
#endif // SERVER_H
