//
// Created by lion on 11/16/24.
//

#ifndef CLIENTHANDLE_H
#define CLIENTHANDLE_H
#include <QThread>
#include <ui_game.h>
#include "client.h"
#include "crazy8.h"
class ClientHandle final : public QThread {
    Q_OBJECT
public:
    explicit ClientHandle(std::unique_ptr<Client> &_client, Ui::Game *_ui) : client(_client), ui(_ui) {
        connect(ui->hosts, &QListWidget::itemClicked, this, &ClientHandle::set_selected_host);
        connect(ui->play, &QPushButton::clicked, this, &ClientHandle::on_play_clicked);
        connect(ui->uncover, &QPushButton::clicked, this, &ClientHandle::on_uncover_clicked);
    }
    void input(int key) const {
        using namespace crazy8;
        QVector<char> message(sizeof(CardPlayed));
        if (key == -2) {
            client->response(message, MessageType::disconnect);
        } else {
            CardPlayed card{};
            card.type = static_cast<int8_t>(key);
            memcpy(message.data(), &card, sizeof(CardPlayed));
            client->response(message, play);
        }
    }
    void set_server_addr() const {
        client->set_server_addr(selected_host);
        long len{};
        std::string key{client->get_key(len)};
        key.append("KYED");
        key.insert(0, "KYST");
        QVector<char> message{key.begin(), key.end()};
        client->response(message, crazy8::MessageType::reply_broadcast);
        client->stop_listen();
    }
public slots:
    void onBroadcastReceived(QVector<char> data) const {
        in_addr addr{};
        addr.s_addr = client->hosts.back();
        const std::string label{inet_ntoa(addr)};
        ui->hosts->addItem(label.c_str());
    }
    void onUpdateReceived(QVector<char> data) {
        decode_message(data);
        ui->ismyturn->clear();
        if (cur_token == my_index) {
            ui->ismyturn->setText("Your Turn");
        } else {
            ui->ismyturn->setText("Others Turn");
        }
    }
    void onPlayerListUpdated(QVector<char> data) const {
        using namespace crazy8;
        PlayersList players_list{};
        memcpy(&players_list, data.data(), sizeof(PlayersList));
        const int number = players_list.number;
        for (int i = 0; i < number; ++i) {
            ui->players->addItem(inet_ntoa(players_list.addr[i]));
        }
    }
    void onStartReceived(QVector<char> data) const { ui->stackedWidget->setCurrentWidget(ui->play_page); }
    void onDealt(QVector<char> data) {
        using namespace crazy8;
        Hand hand{};
        memcpy(&hand, data.data(), sizeof(Hand));
        my_index = hand.index;
        ui->hand->clear();
        for (int i = 0; i < hand.number; ++i) {
            std::string card{suit_name[hand.suit[i]] + ' ' + rank_name[hand.rank[i]]};
            ui->hand->addItem(card.c_str());
        }
    }

private:
    std::unique_ptr<Client> &client;
    Ui::Game *ui;
    in_addr_t selected_host{};
    int my_index = 0;
    int cur_token = 0;
    void decode_message(QVector<char> &data) {
        using namespace crazy8;
        Info info{};
        memcpy(&info, data.data(), sizeof(Info));
        cur_token = info.index;
        const std::string pattern{suit_name[info.suit] + ' ' + rank_name[info.rank]};
        ui->pattern->setText(pattern.c_str());
        for (int i = 0; i < info.number; ++i) {
            if (info.points[i] > -1) {
                ui->points->addItem(
                        ("Score: " + std::to_string(info.points[i]) + " Hands: " + std::to_string(info.hands[i]))
                                .c_str());
            }
        }
    }
    void set_selected_host(const QListWidgetItem *item) {
        client->selected_host_index = ui->hosts->indexFromItem(item).row();
        selected_host = client->hosts.at(client->selected_host_index);
    }
    void on_play_clicked() const {
        if (cur_token == my_index) {
            const int index = ui->hand->currentIndex().row();
            ui->hand->removeItemWidget(ui->hand->currentItem());
            input(index);
        }
    }
    void on_uncover_clicked() const {
        if (cur_token == my_index) {
            input(-1);
        }
    }
};
#endif // CLIENTHANDLE_H
