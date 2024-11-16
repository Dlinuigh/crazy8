#ifndef GAME_H
#define GAME_H

#include "client.h"
#include "server.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class Game;
}
QT_END_NAMESPACE

class Game : public QMainWindow
{
    Q_OBJECT

public:
    Game(QWidget *parent = nullptr);
    ~Game();

private:
    Ui::Game *ui;
    std::unique_ptr<Server> server{nullptr};
    std::unique_ptr<Client> client{nullptr};
    void on_host_pressed();
    void on_join_pressed();
    void on_host_btn_pressed();
    void on_join_btn_pressed();
    void on_cancel_host_pressed();
    void on_cancel_join_pressed();
    void on_start_game_pressed();
    void on_host_cancel_pressed();
    void on_join_cancel_pressedd();
};
#endif // GAME_H
