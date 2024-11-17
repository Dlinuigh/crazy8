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
    }
    void input(int key) const {
        using namespace crazy8;
        QString message;
        if (key == 0) {
            client->response(message, MessageType::uncover);
        } else if (key == -1) {
            client->response(message, MessageType::disconnect);
        } else {
            client->response(message, MessageType::play);
        }
    }
    void set_server_addr() {
        client->set_server_addr(selected_host);
        client->response(QString("Hello!Server"), crazy8::MessageType::reply_broadcast);
        client->stop_listen();
    }
public slots:
    void onBroadcastReceived() const {
        // TODO show host in listitem
        in_addr addr{};
        addr.s_addr = client->hosts.back();
        const QString label{inet_ntoa(addr)};
        ui->hosts->addItem(label);
    }

private:
    std::unique_ptr<Client> &client;
    Ui::Game *ui;
    in_addr_t selected_host{};
    static crazy8::Info decode_message(QString &message) {
        crazy8::Info info{};
        memcpy(&info, message.data(), sizeof(crazy8::Info));
        return info;
    }
    void set_selected_host(QListWidgetItem *item) { selected_host = inet_addr(item->text().toLatin1().data()); }
};
#endif // CLIENTHANDLE_H
