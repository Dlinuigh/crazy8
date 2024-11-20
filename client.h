#ifndef CLIENT_H
#define CLIENT_H

#include <QThread>
#include <arpa/inet.h>
#include <fcntl.h>
#include <fstream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "crazy8.h"
#include "crypto.h"

class Client final : public QThread {
    Q_OBJECT
public:
    std::vector<in_addr_t> hosts{};
    int selected_host_index{0};
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
        sockaddr_in host_addr{};
        socklen_t len = sizeof(host_addr);
        std::string buffer(1024, '\0');
        try {
            if (recvfrom(fd, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr *>(&host_addr), &len) == -1) {
                throw errno;
            }
            if (addr.sin_addr.s_addr != host_addr.sin_addr.s_addr &&
                host_addr.sin_addr.s_addr == server_addr.sin_addr.s_addr) {
                buffer.resize(strlen(buffer.data()));
                if(buffer.compare(0, 4, "KYST") == 0 && buffer.compare(strlen(buffer.data()) - 4, 4, "KYED") == 0) {
                    qDebug("Receive some broadcast sent before.");
                }else {
                    std::string recv_message{};
                    decode(recv_message, buffer);
                    using namespace crazy8;
                    const QVector<char> data{recv_message.begin() + 1, recv_message.end()};
                    switch (static_cast<MessageType>(recv_message[0])) {
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
            }
        } catch (...) {
            if(errno != EAGAIN) {
                qDebug("Error: %s", strerror(errno));
            }
        }
    }
    void response(QVector<char> &data, const crazy8::MessageType type) const {
        try {
            if (type != crazy8::reply_broadcast) {
                crazy8::Message message{};
                message.type = type;
                memcpy(message.data, data.data(), sizeof(data));
                constexpr int len = sizeof(crazy8::Message);
                std::string buffer(len, '\0');
                memcpy(buffer.data(), &message, len);
                std::string send_message{};
                encode(send_message, buffer);
                if (sendto(fd, send_message.data(), send_message.size(), MSG_CONFIRM,
                           reinterpret_cast<const sockaddr *>(&server_addr), addr_len) == -1) {
                    throw errno;
                }
            } else {
                // send key, don't need encode.
                if (sendto(fd, data.data(), data.size(), MSG_CONFIRM, reinterpret_cast<const sockaddr *>(&server_addr),
                           addr_len) == -1) {
                    throw errno;
                }
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
        crypto = std::make_unique<Crypto>();
        while (not should_stop) {
            // mind order, once receive won't receive again.
            if (should_listen) {
                listen();
            } else {
                receive();
            }
            sleep(1);
        }
    }
    void request_stop() {
        should_stop = true;
        qDebug("Stop Client");
    }
    void listen() {
        // crazy8::Message message{};
        std::string buffer(1024, '\0');
        sockaddr_in server{};
        try {
            if (recvfrom(fd, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr *>(&server), &addr_len) == -1) {
                throw errno;
            }
            if (buffer.compare(0, 4, "KYST") == 0 && buffer.compare(strlen(buffer.data()) - 4, 4, "KYED") == 0) {
                if (std::find(hosts.begin(), hosts.end(), server.sin_addr.s_addr) == hosts.end()) {
                    hosts.push_back(server.sin_addr.s_addr);
                    buffer.resize(strlen(buffer.data()));
                    const std::vector<unsigned char> tmp_key(buffer.begin() + 4, buffer.end() - 4);
                    server_keys.push_back(tmp_key);
                    emit Broadcast({});
                } else {
                    qDebug("Already recorded the host addr.");
                }
            }
        } catch (...) {
            qDebug("Error: %s", strerror(errno));
        }
    }
    void set_server_addr(in_addr_t addr) {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = addr;
        server_key = server_keys.at(selected_host_index);
        crypto->set_key(server_key);
        // qDebug("Shared_Secret is %x", crypto->get_shared_secret().c_str());
        std::ofstream outfile("server_keys.txt");
        outfile << crypto->get_shared_secret();
    }

    void stop_listen() { should_listen = false; }
    [[nodiscard]] std::string get_key(long &len) const { return crypto->get_key(len); }

private:
    int fd{0};
    int port{12345};
    bool should_stop{false};
    bool should_listen{true};
    const int max_buffer_size{10};
    sockaddr_in server_addr{}, addr{};
    socklen_t addr_len{sizeof(addr)};
    std::unique_ptr<Crypto> crypto;
    std::vector<unsigned char> server_key{};
    std::vector<std::vector<unsigned char>> server_keys{};
    void encode(std::string &send, std::string &input) const { crypto->AES_Encrypt(input, send); }
    void decode(std::string &recv, std::string &input) const { crypto->AES_Decrypt(input, recv); }
signals:
    void Broadcast(QVector<char> data);
    void Update(QVector<char> data);
    void PlayerList(QVector<char> data);
    void Start(QVector<char> data);
    void Dealt(QVector<char> data);
};
#endif // CLIENT_H
