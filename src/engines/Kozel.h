#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QRandomGenerator>
#include <algorithm>
#include <QJsonObject>
#include <QJsonArray>

#include "Poker.h"

int getKozelCardPoints(int rank);

/**
 * @brief Состояние участника игры в Козла.
 */
struct KozelPlayer {
    int           id = 0;
    QString       name;
    int           avatar = 0;
    QVector<Card> hand;
    int           pointsCollected = 0;
    bool          isBot = false;
    bool          isOut = false;
};

/**
 * @brief Движок правил игры Козёл (Изолирован от графики и сети).
 */
class KozelEngine : public QObject {
    Q_OBJECT
public:
    QVector<Card>             deck;
    Suit                      trumpSuit = Hearts;
    QVector<KozelPlayer>      players;
    QVector<QPair<int, Card>> currentTrick;

    int     leadPlayerIdx    = 0;
    int     currentTurnIdx   = 0;
    bool    gameOver         = false;
    bool    isProcessingMove = false;
    QString statusMessage;
    int     myIdx            = 0;

    QJsonObject toJson(int targetId) const;
    void fromJson(const QJsonObject& json);

    void initGame(int oppCount, bool netGame = false);
    void replenishHands();
    bool isValidMove(int playerIdx, const Card& card) const;
    void playCards(int playerIdx, QVector<int> cardIndices);
    void resolveTrick();
    void checkWinCondition();
    bool makeAiMove();

    int  countActivePlayers();
    void advanceTurn();
    void updateStatus();

    void handlePlayerReconnect(int pIdx);
    void handlePlayerDisconnect(int pIdx);

signals:
    void stateChanged();
};
