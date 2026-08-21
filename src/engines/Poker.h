#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <algorithm>
#include <map>

namespace PokerConfig {
    constexpr int DEFAULT_BALANCE = 1000;
    constexpr int SMALL_BLIND     = 10;
    constexpr int BIG_BLIND       = 20;
}

/**
 * @brief Перечисление карточных мастей.
 */
enum Suit {
    Hearts   = 0, ///< ♥ Червы
    Diamonds = 1, ///< ♦ Бубны
    Clubs    = 2, ///< ♣ Трефы
    Spades   = 3  ///< ♠ Пики
};

/**
 * @brief Структура отдельной игральной карты стандартной колоды.
 */
struct Card {
    Suit suit = Hearts;
    int  rank = 2; ///< Ранг: 2..14 (14 = Туз)

    QJsonObject toJson() const;
    static Card fromJson(const QJsonObject& obj);

    bool operator<(const Card& o) const;
    bool operator==(const Card& o) const;
};

/**
 * @brief Иерархия силы покерных комбинаций Техасского Холдема.
 */
enum HandRank {
    HighCard,      ///< Старшая карта
    Pair,          ///< Пара
    TwoPair,       ///< Две пары
    ThreeOfAKind,  ///< Тройка (Сет)
    Straight,      ///< Стрит
    Flush,         ///< Флеш
    FullHouse,     ///< Фулл-Хаус
    FourOfAKind,   ///< Каре
    StraightFlush  ///< Стрит-Флеш / Роял-Флеш
};

/**
 * @brief Результат расчета силы покерной комбинации.
 */
struct HandValue {
    HandRank     rank  = HighCard;
    unsigned int score = 0;       ///< Уникальный битовый скор для мгновенного численного сравнения
    QString      name  = "";
};

HandValue evaluate5Cards(QVector<Card> c);
HandValue evaluate7Cards(const QVector<Card>& cards);

/**
 * @brief Фазы покерной раздачи.
 */
enum Phase {
    WAITING,
    PREFLOP,
    FLOP,
    TURN,
    RIVER,
    SHOWDOWN
};

inline QString getAvatarEmojiById(int avatarId) {
    switch (avatarId) {
        case 1:  return "💀";
        case 2:  return "♠";
        case 3:  return "🃏";
        case 4:  return "🤖";
        default: return "👑";
    }
}

/**
 * @brief Состояние участника покерного стола.
 */
struct Player {
    int           id = 0;
    QString       name;
    int           avatar = 0;
    QVector<Card> holeCards;
    int           balance          = PokerConfig::DEFAULT_BALANCE;
    int           currentBet       = 0;
    int           totalContributed = 0; ///< Накопленный вклад в раздачу для расчета побочных банков (Side Pots)
    bool          isBot            = false;
    bool          hasFolded        = false;
    bool          isAllIn          = false;
    bool          isBankrupt       = false;
    bool          isDisconnected   = false;
    HandValue     bestHand;
};

/**
 * @brief Движок правил игры Texas Hold'em Poker (Изолирован от графики и сети).
 */
class PokerEngine : public QObject {
    Q_OBJECT
public:
    explicit PokerEngine(QObject* parent = nullptr);

    QVector<Card>   deck;
    QVector<Card>   communityCards;
    QVector<Player> players;

    Phase   phase              = WAITING;
    int     pot                = 0;
    int     currentHighestBet  = 0;
    int     dealerIdx          = 0;
    int     currentTurnIdx     = 0;
    int     smallBlind         = PokerConfig::SMALL_BLIND;
    int     bigBlind           = PokerConfig::BIG_BLIND;
    int     minRaise           = PokerConfig::BIG_BLIND;
    int     playersActed       = 0;

    QString statusMessage;
    bool    gameOver           = false;
    bool    isProcessingMove   = false;
    int     myIdx              = 0;

    void initGame(int oppCount, bool netGame = false);
    int  countSolventPlayers();
    void resetGame();
    void startNewHand();
    int  getNextActivePlayer(int current);
    int  countActivePlayers();
    int  countNonAllInPlayers();
    void placeBet(int pIdx, int amount);
    void handlePlayerDisconnect(int pIdx);
    void handlePlayerReconnect(int pIdx);
    void processAction(int pIdx, const QString& action, int raiseTotal = 0);
    void checkPhaseAdvance();
    void resolveShowdown();
    void updateStatus();
    bool makeAiMove();

    QJsonObject toJson(int targetId) const;
    void fromJson(const QJsonObject& json);

signals:
    void stateChanged();
};
