#include "game.h"
#include "./ui_game.h"

Game::Game(QWidget *parent) : QMainWindow(parent), ui(new Ui::Game) {
    ui->setupUi(this);
    ui->stackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->stackedWidget->setCurrentWidget(ui->main);
    connect(ui->host, &QPushButton::clicked, this, &Game::on_host_pressed);
    connect(ui->join, &QPushButton::clicked, this, &Game::on_join_pressed);
    connect(ui->host_btn, &QPushButton::clicked, this, &Game::on_host_btn_pressed);
    connect(ui->join_btn, &QPushButton::clicked, this, &Game::on_join_btn_pressed);
    connect(ui->start_game, &QPushButton::clicked, this, &Game::on_start_game_pressed);
    // from page
    connect(ui->host_cancel, &QPushButton::clicked, this, &Game::on_host_cancel_pressed);
    connect(ui->join_cancel, &QPushButton::clicked, this, &Game::on_join_cancel_pressedd);
    // from wait
    connect(ui->cancel_host, &QPushButton::clicked, this, &Game::on_cancel_host_pressed);
    connect(ui->cancel_join, &QPushButton::clicked, this, &Game::on_cancel_join_pressed);
}
void Game::on_host_pressed() {
    ui->stackedWidget->setCurrentWidget(ui->host_wait);
    // TODO: host start, expose a port to lan
    max_players = ui->number->value();
    server = std::make_unique<Server>(max_players);
    server->start();
}
void Game::on_join_pressed() {
    ui->stackedWidget->setCurrentWidget(ui->join_wait);
    client_handle->set_server_addr();
}
void Game::on_start_game_pressed() {
    // TODO: start a game between players
    server_handle = std::make_unique<ServerHandle>(server, ui);
    server_handle->start_game();
    connect(server.get(), &Server::PlayerJoin, server_handle.get(), &ServerHandle::onPlayerJoined);
    connect(server.get(), &Server::PlayerPlay, server_handle.get(), &ServerHandle::onPlayerPlay);
}

void Game::on_host_btn_pressed() { ui->stackedWidget->setCurrentWidget(ui->host_page); }
void Game::on_join_btn_pressed() {
    ui->stackedWidget->setCurrentWidget(ui->join_page);
    client = std::make_unique<Client>();
    client->start();
    client_handle = std::make_unique<ClientHandle>(client, ui);
    connect(client.get(), &Client::broadcast, client_handle.get(), &ClientHandle::onBroadcastReceived);
    connect(client.get(), &Client::update, client_handle.get(), &ClientHandle::onUpdateReceived);
}

void Game::on_cancel_host_pressed() {
    ui->stackedWidget->setCurrentWidget(ui->host_page);
    // TODO: cancel host this game. remove all players.
    server->request_stop();
}
void Game::on_cancel_join_pressed() {
    ui->stackedWidget->setCurrentWidget(ui->join_page);
    // TODO: exit this game.
}

void Game::on_host_cancel_pressed() {
    ui->stackedWidget->setCurrentWidget(ui->main);
    // TODO: cancel host this game, not even start expose the port.
}
void Game::on_join_cancel_pressedd() {
    ui->stackedWidget->setCurrentWidget(ui->main);
    // TODO: cancel join a game, not even in a game.
    client->request_stop();
}
Game::~Game() {
    delete ui;
    // if (server)
    //   server->request_stop();
    // if (client)
    //   client->request_stop();
}
