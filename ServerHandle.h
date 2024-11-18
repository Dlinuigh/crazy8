//
// Created by lion on 11/16/24.
//

#ifndef SERVERHANDLE_H
#define SERVERHANDLE_H
#include "crazy8.h"
#include "server.h"

#include <./ui_game.h>
class ServerHandle final : public QThread {
    Q_OBJECT
public:
    explicit ServerHandle(std::unique_ptr<Server> &_server, Ui::Game *_ui) : server(_server), ui(_ui) {
        connect(ui->play,&QPushButton::clicked, this, &ServerHandle::play);
        connect(ui->uncover, &QPushButton::clicked, this, &ServerHandle::uncover);
    }
    void start_game() {
        setup_board();
        send_message(1);
        deal();
        show_pattern();
        send_message(0);
        qDebug("End of start_game function.");
    }
    void add_self_to_game() const {
        server->players_addr.push_back(server->get_addr().sin_addr);
        show_invited_players();
    }
    void show_invited_players() const {
        qDebug() << "show_invited_players";
        ui->invited_players->clear();
        for (auto &player: server->players_addr) {
            ui->invited_players->addItem(inet_ntoa(player));
        }
        notify_others_about_players_list();
    }
    void notify_others_about_players_list() const {
        qDebug() << "notify_others_about_players_list";
        crazy8::PlayersList players_list{};
        players_list.number = server->players_addr.size();
        int i = 0;
        for (auto &player: server->players_addr) {
            players_list.addr[i] = player;
            ++i;
        }
        QVector<char> message;
        message.resize(sizeof(crazy8::Message));
        memcpy(message.data(), &players_list, sizeof(crazy8::PlayersList));
        server->response(message, crazy8::MessageType::notify);
    }
public slots:
    void onPlayerJoined(QVector<char> data) const { show_invited_players(); }
    void onPlayerPlay(QVector<char> data){
        using namespace crazy8;
        CardPlayed card;
        card.type = data[0];
        board->players[token].operate(card.type, *board);
        send_message(0);
    }

private:
    std::unique_ptr<Board> board{nullptr};
    std::unique_ptr<Server> &server;
    Ui::Game *ui;
    // turn token
    int token{0};
    void setup_board() {
        std::vector<Player> players{};
        for (int i = 0; i < server->max_players; ++i) {
            Player tmp{};
            players.emplace_back(tmp);
        }
        board = std::make_unique<Board>(players);
    }
    void send_message(const int type) {
        qDebug() << "send_message";
        using namespace crazy8;
        QVector<char> message(sizeof(Info));
        message.resize(sizeof(Info));
        if (type == 0) {
            message = generate_message();
            server->response(message, info);
            ui->ismyturn->clear();
            if(token == 0) {
                ui->ismyturn->setText("Your Turn");
            }else {
                ui->ismyturn->setText("Others Turn");
            }
            show_pattern();
            if(token < server->players_addr.size()) {
                token+=1;
            }else {
                token=0;
            }
        } else if (type == 1) {
            memcpy(message.data(), "START_GAME", 11);
            server->response(message, MessageType::start);
        } else if (type == -1) {
            memcpy(message.data(), "END_GAME", 9);
            server->response(message, end);
        } else {
            server->response(message, win);
        }
    }
    void play() {
        if(token == 0) {
            board->players[0].operate(ui->hand->currentIndex().row(), *board);
            ui->hand->removeItemWidget(ui->hand->currentItem());
            send_message(0);
        }
    }
    void uncover() {
        if(token == 0) {
            board->players[0].operate(-1, *board);
            send_message(0);
        }
    }
    [[nodiscard]] QVector<char> generate_message() const {
        qDebug() << "generate_message";
        const int number = server->max_players;
        using namespace crazy8;
        Info info{};
        QVector<char> message(sizeof(Info));
        const Card pattern = board->top();
        const auto hands{board->check_players_hand()};
        const auto points{board->check_players_points()};
        info.index = token;
        info.suit = pattern.suit;
        info.rank = pattern.rank;
        for (int i = 0; i < number; ++i) {
            info.hands[i] = hands.at(i);
            info.points[i] = static_cast<int16_t>(points.at(i));
        }
        memcpy(message.data(), &info, sizeof(info));
        return message;
    }
    void deal() const {
        qDebug("Begin Deal Cards");
        using namespace crazy8;
        try {
            board->deal();
            qDebug("End Deal Cards");
            for (int i = 0; i < server->max_players; ++i) {
                QVector<char> data{};
                data.resize(sizeof(Hand));
                generate_hand(data, board->players[i].hand, i);
                server->chat(i, data, MessageType::deal);
            }
            qDebug("End Send Hand info.");
            for (const auto h: board->players.at(0).hand) {
                std::string hand{suit_name[h.suit] + ' ' + rank_name[h.rank]};
                ui->hand->addItem(hand.c_str());
            }
        } catch (...) {
            qDebug("Error: %s", strerror(errno));
        }
    }
    static void generate_hand(QVector<char>& data, const std::vector<Card> &hand, int index) {
        crazy8::Hand hand_data{};
        hand_data.index = index;
        hand_data.number = hand.size();
        for (int i = 0; i < hand.size(); ++i) {
            hand_data.suit[i] = hand.at(i).suit;
            hand_data.rank[i] = hand.at(i).rank;
        }
        memcpy(data.data(), &hand_data, sizeof(crazy8::Hand));
    }
    void show_pattern() const {
        qDebug() << "show_pattern";
        using namespace crazy8;
        std::string pattern{suit_name[board->top().suit] + ' ' + rank_name[board->top().rank]};
        ui->pattern->setText(pattern.c_str());
    }
};
#endif // SERVERHANDLE_H
