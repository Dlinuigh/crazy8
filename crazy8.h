#ifndef CRAZY8_H
#define CRAZY8_H
#include <vector>
#define SUIT_SIZE 4
#define RANK_SIZE 13
#include <algorithm>
#include <random>

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

inline Player::Player() {}

inline void Player::play(const int index, Board &board) {
  // index 经过外面的判断，是一个合理的手牌编号
  board.discard(std::move(hand.at(index)));
  hand.erase(hand.begin() + index);
}

inline void Player::receive(Card &&card) { hand.push_back(card); }

inline bool check(const Card &src, const Card &pat) {
  if (src.suit == pat.suit || src.rank == pat.rank ||
      src.rank == Card::Rank::eight) {
    return true;
  }
  return false;
}

inline void Player::operate(int key, Board &board) {
  Card pattern = board.top();
  if (key > 0 && key <= hand.size()) {
    if (check(hand.at(key), pattern)) {
      play(key, board);
    }
  } else if (key == ' ') {
    board.uncover();
  }
}

inline int Player::count() const { return hand.size(); }

inline int Player::penalty() const {
  int penalty = 0;
  for (auto it = hand.begin(); it != hand.end(); ++it) {
    if (it->rank == Card::eight) {
      penalty += 50;
    } else if (it->rank >= Card::jack && it->rank <= Card::king) {
      penalty += 10;
    } else {
      penalty += it->rank;
    }
  }
  return penalty;
}
inline Board::Board() {}
inline Board::Board(
    const std::vector<std::reference_wrapper<Player>> &_players) {
  score_to_win = 50 * _players.size();
  for (auto p : _players) {
    players.emplace_back(std::move(p));
  }
  shuffle();
}

inline void Board::deal() {
  const int hand_cap = players.size() == 2 ? 7 : 5;
  for (int i = 0; i < hand_cap; ++i) {
    for (auto p : players) {
      p.get().receive(std::move(deck.back()));
      deck.pop_back();
    }
  }
}

inline void Board::uncover() {
  discard_pile.push_back(deck.back());
  deck.pop_back();
}

inline void Board::recreate_deck() {
  // 当deck为空时，调用该函数将除最上面也就是back那张以外的牌重新洗牌加入deck中
  deck.insert(deck.end(), std::make_move_iterator(discard_pile.begin()),
              std::make_move_iterator(discard_pile.end() - 1));
}

inline void Board::re_roll() {
  discard_pile.clear();
  shuffle();
}

inline void Board::calculate(Player &player) {
  int point{0};
  for (auto p : players) {
    if (not p.get().count()) {
      point += p.get().penalty();
    }
  }
  player.add_point(point);
  if (player.get_point() >= score_to_win) {
    champion(player);
  }
}

inline void Board::shuffle() {
  for (int j = 0; j < RANK_SIZE; ++j) {
    for (int i = 0; i < SUIT_SIZE; ++i) {
      deck.emplace_back(
          // static_cast<Card::Suit>(static_cast<std::underlying_type<Card::Suit>::type>(Card::Suit::spade+i)),
          // static_cast<Card::Rank>(static_cast<std::underlying_type<Card::Rank>::type>(Card::Rank::two+j))
          static_cast<Card::Suit>(Card::Suit::spade + i),
          static_cast<Card::Rank>(Card::Rank::two + j));
    }
  }
  std::shuffle(deck.begin(), deck.end(), std::mt19937(std::random_device()()));
}

inline void Board::champion(Player &player) {
  // TODO 游戏迎来胜利者，但是没有任何处理方法，除了重开一局
}

inline Card Board::top() const { return Card{discard_pile.back()}; }

inline void Board::discard(Card &&card) { discard_pile.push_back(card); }
#endif // CRAZY8_H
