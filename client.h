#ifndef CLIENT_H
#define CLIENT_H

#include <QThread>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "crazy8.h"

class Client final : public QThread {
    Q_OBJECT
public:
    std::vector<in_addr_t> hosts{};
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
            }
            if (addr.sin_addr.s_addr != host_addr.sin_addr.s_addr) {
                using namespace crazy8;
                const QVector<char> data{message.data, message.data + sizeof(Info)};
                switch (message.type) {
                    case MessageType::start: {
                        emit Start(data);
                        break;
                    }
                    case end: {
                        break;
                    }
                    case win: {
                        break;
                    }
                    case info: {
                        emit Update(data);
                        break;
                    }
                    case bye: {
                        break;
                    }
                    case notify: {
                        emit PlayerList(data);
                        break;
                    }
                    case deal: {
                        emit Dealt(data);
                        break;
                    }
                    default:;
                }
            }
        } catch (...) {
            qDebug("Error: %s", strerror(errno));
        }
    }
    void response(QVector<char> &data, const crazy8::MessageType type) const {
        try {
            crazy8::Message message{};
            message.type = type;
            memcpy(message.data, data.data(), sizeof(data));
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
            if (message.type == crazy8::MessageType::broadcast) {
                qDebug("Received broadcast message");
                const QVector<char> data{message.data, message.data + sizeof(crazy8::Info)};
                if (std::find(hosts.begin(), hosts.end(), server.sin_addr.s_addr) == hosts.end()) {
                    qDebug("Not even record the host addr.");
                    hosts.push_back(server.sin_addr.s_addr);
                    emit Broadcast(data);
                } else {
                    qDebug("Already recorded the host addr.");
                }
            } else {
                qDebug("Message's type is not broadcast. But %d", message.type);
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
    const int max_buffer_size{10};
    // server addr selected
    sockaddr_in server_addr{}, addr{};
    socklen_t addr_len{sizeof(addr)};
signals:
    void Broadcast(QVector<char> data);
    void Update(QVector<char> data);
    void PlayerList(QVector<char> data);
    void Start(QVector<char> data);
    void Dealt(QVector<char> data);
};
#endif // CLIENT_H
