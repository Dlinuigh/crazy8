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
    connect(ui->join_cancel, &QPushButton::clicked, this, &Game::on_join_cancel_pressed);
    // from wait
    connect(ui->cancel_host, &QPushButton::clicked, this, &Game::on_cancel_host_pressed);
    connect(ui->cancel_join, &QPushButton::clicked, this, &Game::on_cancel_join_pressed);
}
void Game::on_host_pressed() {
    ui->stackedWidget->setCurrentWidget(ui->host_wait);
    max_players = ui->number->value();
    create_server();
    create_server_handle();
    server_handle->add_self_to_game();
    bind_server();
    server->start();
}
void Game::on_join_pressed() const {
    ui->stackedWidget->setCurrentWidget(ui->join_wait);
    client_handle->set_server_addr();
}
void Game::on_start_game_pressed() const {
    server_handle->start_game();
    ui->stackedWidget->setCurrentWidget(ui->play_page);
}

void Game::on_host_btn_pressed() const { ui->stackedWidget->setCurrentWidget(ui->host_page); }
void Game::on_join_btn_pressed() {
    ui->stackedWidget->setCurrentWidget(ui->join_page);
    create_client();
    create_client_handle();
    bind_client();
    client->start();
}

void Game::on_cancel_host_pressed() const {
    ui->stackedWidget->setCurrentWidget(ui->host_page);
    server->request_stop();
}
void Game::on_cancel_join_pressed() const {
    ui->stackedWidget->setCurrentWidget(ui->join_page);
    // TODO: show a notify to others about exit.
}

void Game::on_host_cancel_pressed() const {
    ui->stackedWidget->setCurrentWidget(ui->main);
}
void Game::on_join_cancel_pressed() const {
    ui->stackedWidget->setCurrentWidget(ui->main);
    client->request_stop();
}
void Game::create_server() { server = std::make_unique<Server>(max_players); }
void Game::create_client() { client = std::make_unique<Client>(); }
void Game::create_server_handle() { server_handle = std::make_unique<ServerHandle>(server, ui); }
void Game::create_client_handle() { client_handle = std::make_unique<ClientHandle>(client, ui); }
void Game::bind_server() {
    connect(server.get(), &Server::PlayerJoin, server_handle.get(), &ServerHandle::onPlayerJoined);
    connect(server.get(), &Server::PlayerPlay, server_handle.get(), &ServerHandle::onPlayerPlay);
}
void Game::bind_client() {
    connect(client.get(), &Client::Broadcast, client_handle.get(), &ClientHandle::onBroadcastReceived);
    connect(client.get(), &Client::Update, client_handle.get(), &ClientHandle::onUpdateReceived);
    connect(client.get(), &Client::Start, client_handle.get(), &ClientHandle::onStartReceived);
    connect(client.get(), &Client::PlayerList, client_handle.get(), &ClientHandle::onPlayerListUpdated);
    connect(client.get(), &Client::Dealt, client_handle.get(), &ClientHandle::onDealt);
}
Game::~Game() {
    delete ui;
}
