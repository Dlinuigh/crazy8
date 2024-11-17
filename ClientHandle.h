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
    // TODO: this lazy thread will start only client thread emit a signal.
    // TODO: player input will handle at this place. send them to server, but some detail and restrict here.
public:
    explicit ClientHandle(std::unique_ptr<Client> &_client, Ui::Game *_ui) : client(_client), ui(_ui) {
        connect(ui->hosts, &QListWidget::itemClicked, this, &ClientHandle::set_selected_host);
        connect(ui->play, &QPushButton::clicked, this, &ClientHandle::on_play_clicked);
        connect(ui->uncover, &QPushButton::clicked, this, &ClientHandle::on_uncover_clicked);
    }
    void input(int key) const {
        using namespace crazy8;
        std::string message;
        if (key == -2) {
            client->response(message, MessageType::disconnect);
        } else {
            crazy8::CardPlayed card;
            card.type = static_cast<int8_t>(key);
            memcpy(message.data(), &card, sizeof(crazy8::CardPlayed));
            client->response(message, MessageType::play);
        }
    }
    void set_server_addr() const {
        client->set_server_addr(selected_host);
        client->response(std::string("Hello!Server"), crazy8::MessageType::reply_broadcast);
        client->stop_listen();
    }
public slots:
    void onBroadcastReceived() const {
        // TODO show host in listitem
        in_addr addr{};
        addr.s_addr = client->hosts.back();
        const std::string label{inet_ntoa(addr)};
        ui->hosts->addItem(label.c_str());
    }
    void onUpdateReceived() const {
        decode_message(client->buffer);
    }

private:
    std::unique_ptr<Client> &client;
    Ui::Game *ui;
    in_addr_t selected_host{};
    bool my_turn = false;
    void decode_message(const crazy8::Info &info) const {
        // TODO show info
        std::string pattern{crazy8::suit_name[info.suit]+' '};
        pattern+=+crazy8::rank_name[info.rank];
        ui->pattern->setText(pattern.c_str());
        for(int i=0; i<info.number; ++i) {
            if(info.points[i]>-1) {
                ui->points->addItem(
                        ("Score: " + std::to_string(info.points[i]) + " Hands: " + std::to_string(info.hands[i]))
                                .c_str());
            }
        }
    }
    void set_selected_host(const QListWidgetItem *item) { selected_host = inet_addr(item->text().toLatin1().data()); }
    void on_play_clicked() const {
        const int index = ui->hand->currentIndex().row();
        input(index);
    }
    void on_uncover_clicked() const {
        input(-1);
    }
};
#endif // CLIENTHANDLE_H
