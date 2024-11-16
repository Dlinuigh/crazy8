#ifndef CRAZY8_H
#define CRAZY8_H
#include <vector>
#define SUIT_SIZE 4
#define RANK_SIZE 13
#include "ui.h"

struct Card {
  enum Suit { spade, heart, diamond, club };

  enum Rank {
    ace = 1,
    two,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
    ten,
    jack,
    queen,
    king,
  };

  Suit suit;
  Rank rank;

  Card(const Suit suit, const Rank rank) : suit(suit), rank(rank) {}
};

class Board;

class Player {
public:
  Player();

  Player(Player &&);

  int count() const;

  void add_point(int);

  int get_point() const;

  void receive(Card &&);

  int penalty() const;

  void operate(int, Board &);

private:
  int point{0};
  std::vector<Card> hand{};

  void play(int, Board &);

  void input();

  // TODO
  // 玩家的输入，包含数字按键和字母按键，数字按键为手牌的顺序，字母空格为揭开，

  static bool check(const Card &, const Card &);
};

class Board {
public:
  bool order{false};
  int score_to_win{0};
  Board();
  Board(const std::vector<std::reference_wrapper<Player>> &players);

  Card top() const;

  void discard(Card &&);

  void uncover();

private:
  std::vector<Card> deck{};
  std::vector<Card> discard_pile{};
  std::vector<std::reference_wrapper<Player>> players{};

  void deal();

  void shuffle();

  void recreate_deck();

  void calculate(Player &);

  void re_roll();

  void champion(Player &);
};

class Game {
public:
  Game();
  void run();

private:
  Window window{Window{}};
  Board board{Board{}};
};
#include "crazy8_impl.inl"
#endif // CRAZY8_H
