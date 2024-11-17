#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include <QThread>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "crazy8.h"

class Client : public QThread {
    Q_OBJECT
public:
    std::vector<in_addr_t> hosts{};
    crazy8::Info buffer{};
    Client() {
        try {
            fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (fd == -1) {
                throw std::runtime_error("Socket creation failed");
            }
            fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
            memset(&addr, 0, addr_len);
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = htons(INADDR_ANY);
            if (bind(fd, reinterpret_cast<const sockaddr *>(&addr), addr_len) < 0) {
                throw errno;
            }
            qDebug("Socket Create Success");
        } catch (...) {
            qDebug("Error: %s", strerror(errno));
        }
    }
    void receive() {
        // receive message and decode them. if address is self
        // emit a signal
        sockaddr_in host_addr{};
        socklen_t len = sizeof(host_addr);
        crazy8::Message message{};
        try {
            if (recvfrom(fd, &message, sizeof(crazy8::Message), 0, reinterpret_cast<sockaddr *>(&host_addr), &len) ==
                -1) {
                throw errno;
            } else {
                if (addr.sin_addr.s_addr != host_addr.sin_addr.s_addr) {
                    using namespace crazy8;
                    switch (message.type) {
                        case MessageType::start: {
                            break;
                        }
                        case MessageType::end: {
                            break;
                        }
                        case MessageType::win: {
                            break;
                        }
                        case MessageType::info: {
                            memcpy(&buffer, message.data, sizeof(crazy8::Info));
                            break;
                        }
                        case MessageType::bye: {
                            break;
                        }
                        default:;
                    }
                }
            }
        } catch (...) {
            qDebug("Error: %s", strerror(errno));
        }
    }
    void response(std::string data, const crazy8::MessageType type) const {
        try {
            crazy8::Message message{};
            message.type = type;
            memcpy(message.data, data.data(), sizeof(message.data));
            constexpr int len = sizeof(crazy8::Message);
            char buffer[len];
            memcpy(buffer, &message, len);
            if (sendto(fd, buffer, len, MSG_CONFIRM, reinterpret_cast<const sockaddr *>(&server_addr), addr_len) ==
                -1) {
                throw errno;
            }
        } catch (...) {
            qDebug("Error: %s", strerror(errno));
        }
    }
    ~Client() override {
        qDebug("Server Will Stop in most 1s");
        close(fd);
        if (not should_stop) {
            request_stop();
            sleep(1);
        }
    }
    void run() override {
        while (not should_stop) {
            // mind order, once receive won't receive again.
            if (should_listen) {
                listen();
            }
            receive();
            sleep(1);
        }
    }
    void request_stop() {
        should_stop = true;
        qDebug("Stop Client");
    }
    void listen() {
        crazy8::Message message{};
        sockaddr_in server{};
        try {
            if (recvfrom(fd, &message, sizeof(crazy8::Message), 0, reinterpret_cast<sockaddr *>(&server), &addr_len) ==
                -1) {
                throw errno;
            }
            if (message.type == crazy8::MessageType::broadcast &&
                std::find(hosts.begin(), hosts.end(), server.sin_addr.s_addr) == hosts.end()) {
                qDebug("Receive a Broadcast");
                hosts.push_back(server.sin_addr.s_addr);
                emit broadcast();
            }
            // TODO if this server selected to join then send a message to server for confirming.
        } catch (...) {
            qDebug("Error: %s", strerror(errno));
        }
    }
    void set_server_addr(in_addr_t addr) {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = addr;
    }
    void stop_listen() { should_listen = false; }

private:
    int fd{0};
    int port{12345};
    bool should_stop{false};
    bool should_listen{true};
    // server addr selected
    sockaddr_in server_addr{}, addr{};
    socklen_t addr_len{sizeof(addr)};
signals:
    void broadcast();
    void update();
};
#endif // CLIENT_H
