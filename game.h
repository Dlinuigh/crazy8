#ifndef GAME_H
#define GAME_H

#include "client.h"
#include "server.h"

#include <QMainWindow>

#include "ClientHandle.h"
#include "ServerHandle.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Game;
}
QT_END_NAMESPACE

class Game final : public QMainWindow
{
    Q_OBJECT

public:
    explicit Game(QWidget *parent = nullptr);
    ~Game() override;

private:
    Ui::Game *ui;
    int max_players{0};
    std::unique_ptr<Server> server{nullptr};
    std::unique_ptr<Client> client{nullptr};
    std::unique_ptr<ServerHandle> server_handle{nullptr};
    std::unique_ptr<ClientHandle> client_handle{nullptr};
    void on_host_pressed();
    void on_join_pressed() const;
    void on_host_btn_pressed() const;
    void on_join_btn_pressed();
    void on_cancel_host_pressed() const;
    void on_cancel_join_pressed() const;
    void on_start_game_pressed() const;
    void on_host_cancel_pressed() const;
    void on_join_cancel_pressed() const;

    void create_server();
    void create_client();
    void create_server_handle();
    void create_client_handle();
    void bind_server();
    void bind_client();
};
#endif // GAME_H
