//
// Created by lion on 11/16/24.
//

#ifndef CLIENTHANDLE_H
#define CLIENTHANDLE_H
#include <QThread>
#include <ui_game.h>

#include "crazy8.h"
class ClientHandle final : public QThread {
    Q_OBJECT
    // TODO: this lazy thread will start only client thread emit a signal.
    // TODO: player input will handle at this place. send them to server, but some detail and restrict here.
public:
    explicit ClientHandle(std::unique_ptr<Client>& _client) : client(_client) {}
    void input(int key) const {
        QString message;
        if(key==0) {
            client->response(message, MessageType::uncover);
        }else if(key==-1) {
            client->response(message, MessageType::disconnect);
        }else{
            client->response(message, MessageType::play);
        }
    }
private:
    std::unique_ptr<Client>& client;
    void set_info(Ui::Game *ui) {

    }
    static Info decode_message(QString &message) {
        Info info{};
        memcpy(&info, message.data(), sizeof(Info));
        return info;
    }
};
#endif // CLIENTHANDLE_H
