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
namespace crazy8 {
    inline std::vector<std::string> suit_name = {"Spade", "Heart", "Diamond", "Club"};
    inline std::vector<std::string> rank_name = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};
    enum MessageType : int8_t {
        info, // server's message
        start,
        end,
        win, // notify everyone about winner.
        bye,
        broadcast,
        deal, // send deal info to client.
        notify, // notify player list.

        play, // player's game operate
        disconnect,
        reply_broadcast,
    };
#pragma pack(push, 1)
    // size: 32 bytes
    struct Info {
        uint8_t number{};
        int8_t suit{};
        int8_t rank{};
        int8_t hands[7]{};
        int16_t points[7]{-1, -1, -1, -1, -1, -1, -1};
        uint8_t index{};
        int8_t _empty[7]{};
    };
    struct Message {
        MessageType type;
        int8_t data[sizeof(Info)];
    };
    struct Hand {
        uint8_t index;
        uint8_t number;
        int8_t suit[7];
        int8_t rank[7];
        int8_t _empty[16];
    };
    struct CardPlayed {
        int8_t type{}; // card or uncover
        int8_t _empty[31]{};
    };
    struct PlayersList {
        uint8_t number{};
        in_addr addr[7]{};
        int8_t empty[3]{};
    };
#pragma pack(pop)
} // namespace crazy8


class Board;

class Player {
public:
    std::vector<Card> hand{};
    [[nodiscard]] int8_t count() const { return static_cast<int8_t>(hand.size()); };

    void add_point(int p) { point += p; }

    [[nodiscard]] int get_point() const { return point; }

    void receive(const Card &card) { hand.push_back(card); }

    [[nodiscard]] int penalty() const {
        int penalty = 0;
        for (const auto it: hand) {
            if (it.rank == Card::eight) {
                penalty += 50;
            } else if (it.rank >= Card::jack && it.rank <= Card::king) {
                penalty += 10;
            } else {
                penalty += it.rank;
            }
        }
        return penalty;
    }

    void operate(int, Board &);

private:
    int point{0};
    void play(int, Board &);

    void input();
    static bool check(const Card &src, const Card &pat) {
        if (src.suit == pat.suit || src.rank == pat.rank || src.rank == Card::Rank::eight) {
            return true;
        }
        return false;
    }
};

class Board {
public:
    bool order{false};
    int score_to_win{0};
    std::vector<Player> players{};
    Board() = default;
    explicit Board(const std::vector<Player> &_players) {
        score_to_win = 50 * static_cast<int>(_players.size());
        for (auto &p: _players) {
            players.emplace_back(p);
        }
        shuffle();
    }
    // check the top of discard pile
    [[nodiscard]] Card top() const { return discard_pile.back(); }
    // player discard hand
    void discard(const Card &card) { discard_pile.push_back(card); }
    // player uncover a card from deck
    void uncover() {
        discard_pile.push_back(deck.back());
        deck.pop_back();
    }
    [[nodiscard]] std::vector<int8_t> check_players_hand() const {
        std::vector<int8_t> hand;
        for (auto &p: players) {
            hand.push_back(p.count());
        }
        return hand;
    }
    [[nodiscard]] std::vector<int> check_players_points() const {
        std::vector<int> points;
        for (auto &p: players) {
            points.push_back(p.get_point());
        }
        return points;
    }
    void deal() {
        const int hand_cap = players.size() == 2 ? 7 : 5;
        for (int i = 0; i < hand_cap; ++i) {
            for (auto &p: players) {
                p.receive(deck.back());
                deck.pop_back();
            }
        }
        uncover();
    }

private:
    std::vector<Card> deck{};
    std::vector<Card> discard_pile{};

    void shuffle() {
        for (int j = 0; j < RANK_SIZE; ++j) {
            for (int i = 0; i < SUIT_SIZE; ++i) {
                deck.emplace_back(static_cast<Card::Suit>(Card::Suit::spade + i),
                                  static_cast<Card::Rank>(Card::Rank::two + j));
            }
        }
        std::shuffle(deck.begin(), deck.end(), std::mt19937(std::random_device()()));
    }

    void recreate_deck() {
        // 当deck为空时，调用该函数将除最上面也就是back那张以外的牌重新洗牌加入deck中
        deck.insert(deck.end(), std::make_move_iterator(discard_pile.begin()),
                    std::make_move_iterator(discard_pile.end() - 1));
    }
    void calculate(Player &player) {
        int point{0};
        for (auto &p: players) {
            if (not p.count()) {
                point += p.penalty();
            }
        }
        player.add_point(point);
        if (player.get_point() >= score_to_win) {
            champion(player);
        }
    }

    void re_roll() {
        discard_pile.clear();
        shuffle();
    }

    void champion(Player &player) {
        // TODO show a comfirm dialog to continue play.
    }
};
inline void Player::play(const int index, Board &board) {
    // index 经过外面的判断，是一个合理的手牌编号
    board.discard(hand.at(index));
    hand.erase(hand.begin() + index);
}
inline void Player::operate(const int key, Board &board) {
    const Card pattern = board.top();
    if (key > 0 && key <= hand.size()) {
        if (check(hand.at(key), pattern)) {
            play(key, board);
        }
    } else if (key == -1) {
        board.uncover();
    }
}
#endif // CRAZY8_H
