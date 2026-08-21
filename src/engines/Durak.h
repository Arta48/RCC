#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QRandomGenerator>
#include <QJsonObject>
#include <QJsonArray>

#include "Poker.h"

/**
 * @brief Структура пары карт на столе Подкидного Дурака (Карта атаки и Карта защиты).
 */
struct DurakTablePair {
    Card attack;
    Card defend;
    bool isDefended = false;
};

/**
 * @brief Состояние участника игры в Дурака.
 */
struct DurakPlayer {
    int           id = 0;
    QString       name;
    int           avatar = 0;
    QVector<Card> hand;
    bool          isBot = false;
    bool          isOut = false;
};

/**
 * @brief Движок правил игры Подкидной Дурак (Изолирован от графики и сети).
 */
class DurakEngine : public QObject {
    Q_OBJECT
public:
    QVector<Card>           deck;
    Card                    trumpCard;
    QVector<DurakPlayer>    players;
    QVector<DurakTablePair> table;
    int                     bitoCount = 0;

    int     attackerIdx      = 0;
    int     defenderIdx      = 1;
    int     currentTurnIdx   = 0;
    bool    isDefenderTaking = false;
    bool    gameOver         = false;
    bool    isProcessingMove = false;
    QString statusMessage;
    int     myIdx            = 0;

    void initGame(int oppCount, bool netGame = false);
    void replenishHands();
    bool canAttackWith(const Card& card) const;
    bool playAttackCard(int playerIdx, int cardHandIdx);
    bool playDefendCard(int playerIdx, int cardHandIdx, int tableIdx);
    void passAction();
    void takeAction();
    void checkWinCondition();
    bool makeAiMove();

    int  countActivePlayers();
    int  getNextActivePlayer(int current);
    void updateStatus();

    void handlePlayerReconnect(int pIdx);
    void handlePlayerDisconnect(int pIdx);

    QJsonObject toJson(int targetId) const;
    void fromJson(const QJsonObject& json);

signals:
    void stateChanged();
};
