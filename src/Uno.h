#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QRandomGenerator>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

#include "AppSettings.h"

enum UnoColor {
    UnoRed    = 0,
    UnoYellow = 1,
    UnoGreen  = 2,
    UnoBlue   = 3,
    UnoWild   = 4
};

enum UnoValue {
    UnoZero = 0, UnoOne, UnoTwo, UnoThree, UnoFour,
    UnoFive, UnoSix, UnoSeven, UnoEight, UnoNine,
    UnoSkip = 10,
    UnoReverse = 11,
    UnoDrawTwo = 12,
    UnoWildCard = 13,
    UnoWildDrawFour = 14
};

struct UnoCard {
    UnoColor color = UnoRed;
    UnoValue value = UnoZero;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["color"] = static_cast<int>(color);
        obj["value"] = static_cast<int>(value);
        return obj;
    }

    static UnoCard fromJson(const QJsonObject& obj) {
        return UnoCard{ static_cast<UnoColor>(obj["color"].toInt(0)), static_cast<UnoValue>(obj["value"].toInt(0)) };
    }

    bool operator==(const UnoCard& o) const {
        return color == o.color && value == o.value;
    }
};

struct UnoPlayer {
    int id = 0;
    QString name;
    int avatar = 0;
    QVector<UnoCard> hand;
    bool isBot = false;
    bool isOut = false;
    bool saidUno = false;
};

class UnoEngine : public QObject {
    Q_OBJECT
public:
    QVector<UnoCard>   deck;
    QVector<UnoCard>   discardPile;
    QVector<UnoPlayer> players;

    UnoColor    currentColor             = UnoRed;
    int         currentTurnIdx           = 0;
    int         direction                = 1;
    int         accumulatedPenalty       = 0;
    bool        hasDrawnThisTurn         = false;
    bool        gameOver                 = false;
    bool        isProcessingMove         = false;
    QString     statusMessage;
    int         myIdx                    = 0;

    int         unoVulnerablePlayerIdx   = -1; // Игрок, которого можно поймать
    qint64      unoVulnerabilityDeadline = 0;  // Дедлайн окна форы (мс)

    UnoDrawMode drawMode                 = UnoDrawMode::DrawUntilMatch;
    bool        stackingEnabled          = true;

    void initGame(int oppCount, bool netGame = false);
    void createDeck();
    void replenishDeckIfNeeded();
    bool canPlayCard(const UnoCard& card) const;
    bool playCard(int playerIdx, int cardHandIdx, UnoColor chosenColor = UnoRed, bool callUno = false);
    bool drawCard(int playerIdx);
    void passTurn(int playerIdx);
    void advanceTurn(int step = 1);
    void checkWinCondition();
    bool checkStalemate();
    bool makeAiMove();
    int  getNextActivePlayer(int current, int step = 1);
    int  countActivePlayers();
    void updateStatus();

    bool declareUno(int playerIdx);
    bool catchUno(int catcherIdx, int targetIdx);
    bool hasPlayableCard(int playerIdx) const;

    void handlePlayerReconnect(int pIdx);
    void handlePlayerDisconnect(int pIdx);

    QJsonObject toJson(int targetId) const;
    void fromJson(const QJsonObject& json);

signals:
    void stateChanged();
};
