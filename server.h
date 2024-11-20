#ifndef SERVER_H
#define SERVER_H
#include <QThread>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "crazy8.h"
#include "crypto.h"

class Server final : public QThread {
    Q_OBJECT
public:
    int max_players{0};
    std::vector<in_addr> players_addr{};
    Server() = default;
    explicit Server(int _max_players) : max_players(_max_players) {
        try {
            if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
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
        crypto = std::make_unique<Crypto>();
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
        long len{0};
        std::string key = crypto->get_key(len);
        key.insert(0, "KYST");
        key.append("KYED");
        sendto(fd, key.data(), key.size(), MSG_CONFIRM, reinterpret_cast<const sockaddr *>(&broadcast_addr), addr_len);
    }
    void receive() {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        std::string buffer(1024, '\0');
        try {
            if (recvfrom(fd, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr *>(&client_addr), &len) == -1) {
                throw errno;
            }
            buffer.resize(strlen(buffer.data()));
            if (addr.sin_addr.s_addr != client_addr.sin_addr.s_addr) {
                using namespace crazy8;
                if (buffer.compare(0, 4, "KYST") == 0 &&
                    buffer.compare(static_cast<int>(strlen(buffer.data())) - 4, 4, "KYED") == 0) {
                    players_addr.push_back(client_addr.sin_addr);
                    std::vector<unsigned char> tmp_key(buffer.begin() + 4, buffer.end() - 4);
                    keys.push_back(tmp_key);
                    emit PlayerJoin({});
                    qDebug("Player %s Joined", inet_ntoa(client_addr.sin_addr));
                } else {
                    const int index =
                            static_cast<int>(std::find_if(players_addr.begin(), players_addr.end(),
                                                          [client_addr](const in_addr &addr) {
                                                              return addr.s_addr == client_addr.sin_addr.s_addr;
                                                          }) -
                                             players_addr.begin());
                    std::string recv_message{};
                    crypto->set_key(keys.at(index));
                    std::string _tmp_input{buffer.begin(), buffer.end()};
                    decode(recv_message, _tmp_input);
                    const QVector<char> data{recv_message.begin() + 1, recv_message.end()};
                    switch (static_cast<MessageType>(recv_message[0])) {
                        case MessageType::play: {
                            emit PlayerPlay(data);
                            break;
                        }
                        case MessageType::disconnect: {
                            break;
                        }
                        default:;
                    }
                }
            }
            if (players_addr.size() == max_players) {
                should_broadcast = false;
                qDebug("Stop Broadcast.");
            }
        } catch (...) {
            if(errno!=EAGAIN) {
                qDebug("Error: %s", strerror(errno));
            }
        }
    }
    void response(QVector<char> &data, const crazy8::MessageType type) const {
        for (int i = 0; i < players_addr.size(); ++i) {
            chat(i, data, type);
        }
    }
    void chat(const int index, QVector<char> &data, const crazy8::MessageType type) const {
        if (players_addr[index].s_addr == addr.sin_addr.s_addr) {
            return;
        }
        // host always be the No.0 so index - 1 will be players index.
        crypto->set_key(keys.at(index-1));
        std::ofstream ofs("secret_key.txt");
        ofs << crypto->get_shared_secret();
        qDebug("Char with No.%d player", index);
        crazy8::Message message{};
        message.type = type;
        memcpy(message.data, data.data(), sizeof(message.data));
        constexpr int len = sizeof(message);
        std::string buffer(len, '\0');
        memcpy(buffer.data(), &message, len);

        std::string send_message{};
        encode(send_message, buffer);

        try {
            sockaddr_in client_addr{};
            client_addr.sin_family = AF_INET;
            client_addr.sin_port = htons(port);
            client_addr.sin_addr = players_addr[index];
            if (sendto(fd, send_message.data(), send_message.size(), 0,
                       reinterpret_cast<const sockaddr *>(&client_addr), addr_len) == -1) {
                throw errno;
            }
        } catch (...) {
            qDebug("Error: %s", strerror(errno));
        }
    }
    [[nodiscard]] sockaddr_in get_addr() const { return addr; }

private:
    int fd{};
    bool should_stop{false};
    bool should_broadcast{true};
    int max_buffer_size = 10;
    sockaddr_in addr{}, broadcast_addr{};
    uint16_t port{12345};
    socklen_t addr_len{sizeof(addr)};
    std::unique_ptr<Crypto> crypto;
    std::vector<std::vector<unsigned char>> keys;
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
    void encode(std::string &send, std::string &input) const { crypto->AES_Encrypt(input, send); }
    void decode(std::string &recv, std::string &input) const { crypto->AES_Decrypt(input, recv); }
signals:
    void PlayerJoin(QVector<char> data);
    void PlayerPlay(QVector<char> data);
};
#endif // SERVER_H
