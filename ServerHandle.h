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
        deal();
    }
public slots:
    void onPlayerJoined() const {
        crazy8::Message message{server->buffer.front()};
        server->buffer.pop();
        const std::string label{inet_ntoa(server->players_addr.back())};
        ui->invited_players->addItem(label.c_str());
    }
    void onPlayerPlay() const {
        crazy8::Message message{server->buffer.front()};
        server->buffer.pop();
        if(message.type == crazy8::MessageType::play) {
            crazy8::CardPlayed card;
            card.type = static_cast<int8_t>(message.data[0]);
            board->players[token].get().operate(card.type, *board);
            send_message(0);
        }
    }

private:
    std::unique_ptr<Board> board{nullptr};
    std::unique_ptr<Server>& server;
    Ui::Game *ui;
    // turn token
    int token{0};
    void setup_board() {
        std::vector<std::reference_wrapper<Player>> players;
        for(int i=0; i<server->max_players; ++i) {
            Player tmp{};
            players.emplace_back(tmp);
        }
        board = std::make_unique<Board>(players);
    }
    void send_message(const int type) const {
        using namespace crazy8;
        std::string message;
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
    [[nodiscard]] std::string generate_message() const {
        int number = server->max_players;
        crazy8::Info info{};
        std::string message{};
        Card pattern = board->top();
        auto hands{board->check_players_hand()};
        auto points{board->check_players_points()};
        info.index = token;
        info.suit = pattern.suit;
        info.rank = pattern.rank;
        for(int i=0; i<number; ++i) {
            info.hands[i] = static_cast<short>(hands.at(i));
            info.points[i] = points.at(i);
        }
        memcpy(message.data(), &info, sizeof(info));
        return message;
    }
    void deal() const {
        board->deal();
        for (int i = 0; i < server->max_players; ++i) {
            std::string data{};
            generate_hand(data, board->players[i].get().hand);
            server->chat(i, data, crazy8::MessageType::deal);
        }
    }
    static void generate_hand(std::string &data, const std::vector<Card> &hand) {
        crazy8::Hand hand_data{};
        for(int i=0; i<hand.size();++i) {
            hand_data.index = i;
            hand_data.suit[i] = static_cast<uint8_t>(hand.at(i).suit);
            hand_data.rank[i] = static_cast<uint8_t>(hand.at(i).rank);
        }
        memcpy(data.data(), &hand_data, sizeof(hand_data));
    }
};
#endif // SERVERHANDLE_H
