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
    // TODO only start when Server emit a signal.
    // TODO board store here.
public:
    explicit ServerHandle(std::unique_ptr<Server> &_server, Ui::Game *_ui) : server(_server), ui(_ui) {}
    void start_game() {
        setup_board();
        send_message(1);
    }
public slots:
    void onPlayerJoined() {
        QString label{inet_ntoa(server->players_addr.back())};
        ui->invited_players->addItem(label);
    }

private:
    std::unique_ptr<Board> board{nullptr};
    std::unique_ptr<Server>& server;
    Ui::Game *ui;
    void setup_board() { board = std::make_unique<Board>(); }
    void send_message(const int type) const {
        using namespace crazy8;
        QString message;
        if(type == 0) {
            message = generate_message();
            server->response(message, MessageType::info);
        }else if(type == 1) {
            message = "START_GAME";
            server->response(message, MessageType::start);
        }else if(type == -1) {
            message = "END_GAME";
            server->response(message, MessageType::end);
        }else {
            server->response(message, MessageType::win);
        }
    }
    [[nodiscard]] QString generate_message() const {
        int number = server->max_players;
        crazy8::Info info{};
        QString message{};
        Card pattern = board->top();
        auto hands{board->check_players_hand()};
        auto points{board->check_players_points()};
        memcpy(info.start, "START", sizeof(info.start));
        memcpy(info.end, "END", sizeof(info.end));
        info.suit = pattern.suit;
        info.rank = pattern.rank;
        for(int i=0; i<number; ++i) {
            info.hands[i] = static_cast<short>(hands.at(i));
            info.points[i] = points.at(i);
        }
        memcpy(message.data(), &info, sizeof(info));
        return message;
    }
};
#endif // SERVERHANDLE_H
