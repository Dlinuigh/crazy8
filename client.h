#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>

#include <QString>
#include <QThread>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "crazy8.h"

class Client : public QThread {
public:
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
                throw std::runtime_error("Bind failed");
            }
            qDebug("Socket Create Success");
        } catch (std::runtime_error &e) {
            qDebug("Error: %s", e.what());
        }
    }
    void receive() {
        // receive message and decode them. if address is self
        // emit a signal
    }
    void response(QString data, const MessageType type) const {
        // TODO move to handle.
        try {
            Message message{};
            message.type = type;
            memcpy(message.data, data.data(), sizeof(message.data));
            constexpr int len = sizeof(message);
            char buffer[len];
            memcpy(buffer, &message, len);
            if (sendto(fd, buffer, len, MSG_CONFIRM, reinterpret_cast<const sockaddr *>(&server_addr), addr_len) ==
                -1) {
                throw std::runtime_error("Send failed");
            }
        } catch (std::runtime_error &e) {
            qDebug("Error: %s", e.what());
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
            if (should_listen) {
                listen();
            }
            sleep(1);
        }
    }
    void request_stop() {
        should_stop = true;
        qDebug("Stop Client");
    }
    void listen() {
        try {
            if (recvfrom(fd, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr *>(&server_addr), &addr_len)) {
                if (errno == EAGAIN) {

                } else {
                    throw std::runtime_error("Receive failed");
                }
            } else {
                buffer.push_back(QChar('\0'));
                qDebug("Receive BroadCast Message: %p", buffer.data());
                response();
                should_listen = false;
            }
        } catch (std::runtime_error &e) {
            qDebug("Error: %s", e.what());
        }
    }

private:
    int fd{0};
    int port{12345};
    bool should_stop{false};
    bool should_listen{true};
    sockaddr_in server_addr{}, addr{};
    int buffer_size{1024};
    QString buffer{};
    socklen_t addr_len{sizeof(addr)};
};
#endif // CLIENT_H
