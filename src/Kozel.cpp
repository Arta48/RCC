#include "Kozel.h"
#include "Audio.h"
#include "AppSettings.h"

int getKozelCardPoints(int rank) {
    if (rank == 14) return 11;
    if (rank == 10) return 10;
    if (rank == 13) return 4;
    if (rank == 12) return 3;
    if (rank == 11) return 2;
    return 0;
}

void KozelEngine::initGame(int oppCount, bool netGame) {
    gameOver = false;
    isProcessingMove = false;
    currentTrick.clear();
    deck.clear();
    players.clear();

    KozelPlayer human;
    human.id = 0;
    human.name = AppSettings::instance().nickname;
    human.avatar = static_cast<int>(AppSettings::instance().avatar);
    human.isBot = false;
    players.append(human);

    for (int i = 1; i <= oppCount; ++i) {
        KozelPlayer p;
        p.id = i;
        p.name = netGame ? QString(getLocalizedText("Игрок %1", "Player %1")).arg(i + 1) : QString(getLocalizedText("Бот %1", "Bot %1")).arg(i);
        p.avatar = netGame ? 0 : 4;
        p.isBot = !netGame;
        players.append(p);
    }

    for (int s = 0; s < 4; ++s) {
        for (int r = 6; r <= 14; ++r) {
            deck.append(Card{ static_cast<Suit>(s), r });
        }
    }

    for (int i = deck.size() - 1; i > 0; --i) {
        int j = QRandomGenerator::global()->bounded(i + 1);
        deck.swapItemsAt(i, j);
    }

    trumpSuit = deck.last().suit;
    replenishHands();

    leadPlayerIdx = 0;
    currentTurnIdx = 0;
    statusMessage = getLocalizedText("Ваш ход! Заходите любой картой.", "Your turn! Lead with any card.");
    emit stateChanged();
}

void KozelEngine::replenishHands() {
    for (auto& p : players) {
        if (p.isOut) continue;

        while (p.hand.size() < 4 && !deck.isEmpty()) {
            p.hand.append(deck.takeFirst());
        }
    }
}

bool KozelEngine::isValidMove(int playerIdx, const Card& card) const {
    if (playerIdx != currentTurnIdx) return false;
    if (currentTrick.isEmpty()) return true;

    Suit leadSuit = currentTrick.first().second.suit;
    auto& hand = players[playerIdx].hand;

    bool hasLeadSuit = false;
    for (const auto& c : hand) if (c.suit == leadSuit) hasLeadSuit = true;

    if (hasLeadSuit && card.suit != leadSuit) return false;
    return true;
}

void KozelEngine::playCards(int playerIdx, QVector<int> cardIndices) {
    if (playerIdx != currentTurnIdx || gameOver || cardIndices.isEmpty()) return;

    std::sort(cardIndices.rbegin(), cardIndices.rend());

    bool playedAny = false;
    for (int idx : cardIndices) {
        if (idx >= 0 && idx < players[playerIdx].hand.size()) {
            Card c = players[playerIdx].hand[idx];
            if (isValidMove(playerIdx, c)) {
                players[playerIdx].hand.removeAt(idx);
                currentTrick.append({ playerIdx, c });
                playedAny = true;
            }
        }
    }

    if (!playedAny && !players[playerIdx].hand.isEmpty()) {
        for (int i = 0; i < players[playerIdx].hand.size(); ++i) {
            Card c = players[playerIdx].hand[i];
            if (isValidMove(playerIdx, c)) {
                players[playerIdx].hand.removeAt(i);
                currentTrick.append({ playerIdx, c });
                playedAny = true;
                break;
            }
        }
    }

    if (!playedAny) return;

    int cardsPerPlayer = 1;
    if (!currentTrick.isEmpty()) {
        int firstPlayerId = currentTrick.first().first;
        cardsPerPlayer = 0;
        for (const auto& p : currentTrick) {
            if (p.first == firstPlayerId) cardsPerPlayer++;
        }
    }

    AudioManager::instance().playSound(SoundEffect::CardPlace);

    if (currentTrick.size() >= cardsPerPlayer * countActivePlayers()) {
        resolveTrick();
    } else {
        advanceTurn();
    }
    emit stateChanged();
}

void KozelEngine::resolveTrick() {
    Suit leadSuit = currentTrick.first().second.suit;
    int bestIdx = 0;
    Card bestCard = currentTrick.first().second;
    int trickPoints = 0;

    for (int i = 0; i < currentTrick.size(); ++i) {
        Card c = currentTrick[i].second;
        trickPoints += getKozelCardPoints(c.rank);

        if (c.suit == trumpSuit && bestCard.suit != trumpSuit) {
            bestCard = c; bestIdx = i;
        } else if (c.suit == bestCard.suit && c.rank > bestCard.rank) {
            bestCard = c; bestIdx = i;
        }
    }

    int winnerPlayerIdx = currentTrick[bestIdx].first;
    players[winnerPlayerIdx].pointsCollected += trickPoints;

    currentTrick.clear();
    replenishHands();

    leadPlayerIdx = winnerPlayerIdx;
    currentTurnIdx = winnerPlayerIdx;

    checkWinCondition();
    if (!gameOver) {
        statusMessage = QString(getLocalizedText("%1 забирает взятку (+%2 очков) и ходит!", "%1 takes trick (+%2 pts) and leads!")).arg(players[winnerPlayerIdx].name).arg(trickPoints);
        if (players[currentTurnIdx].hand.isEmpty()) {
            advanceTurn();
        }
    }

}

void KozelEngine::checkWinCondition() {
    bool allEmpty = true;
    for (const auto& p : players) if (!p.hand.isEmpty()) allEmpty = false;

    if (deck.isEmpty() && allEmpty) {
        gameOver = true;
        AudioManager::instance().playSound(SoundEffect::Win);

        int maxPoints = -1;
        QVector<int> winners;

        for (int i = 0; i < players.size(); ++i) {
            if (players[i].pointsCollected > maxPoints) {
                maxPoints = players[i].pointsCollected;
                winners.clear();
                winners.append(i);
            } else if (players[i].pointsCollected == maxPoints) {
                winners.append(i);
            }
        }

        if (maxPoints >= 60) {
            if (winners.size() == 1) {
                statusMessage = QString(getLocalizedText("ПОБЕДА! %1 набрал %2 очков!", "VICTORY! %1 scored %2 points!")).arg(players[winners[0]].name).arg(maxPoints);
            } else {
                statusMessage = QString(getLocalizedText("НИЧЬЯ! Игроки набрали по %1 очков (Раздел)!", "DRAW! Players tied at %1 points!")).arg(maxPoints);
            }
        } else {
            statusMessage = getLocalizedText("Никто не набрал 60 очков! Раздел!", "No one reached 60 points! Split!");
        }

    }
}

bool KozelEngine::makeAiMove() {
    if (isProcessingMove || gameOver) return false;

    isProcessingMove = true;

    if (players[currentTurnIdx].hand.isEmpty()) {
        advanceTurn();
        isProcessingMove = false;
        return false;
    }

    if (players[currentTurnIdx].isBot) {
        auto& hand = players[currentTurnIdx].hand;

        int requiredCardCount = 1;
        if (!currentTrick.isEmpty()) {
            int firstPlayerId = currentTrick.first().first;
            requiredCardCount = 0;
            for (const auto& p : currentTrick) {
                if (p.first == firstPlayerId) requiredCardCount++;
            }
        }

        QVector<int> toPlay;
        for (int i = 0; i < hand.size(); ++i) {
            if (isValidMove(currentTurnIdx, hand[i])) {
                toPlay.append(i);
                if (toPlay.size() == requiredCardCount) break;
            }
        }

        if (toPlay.size() < requiredCardCount) {
            toPlay.clear();
            for (int i = 0; i < hand.size() && toPlay.size() < requiredCardCount; ++i) {
                toPlay.append(i);
            }
        }

        if (!toPlay.isEmpty()) {
            playCards(currentTurnIdx, toPlay);
            isProcessingMove = false;
            return true;
        } else {
            advanceTurn();
            isProcessingMove = false;
            return true;
        }
    }

    isProcessingMove = false;
    return false;
}

int KozelEngine::countActivePlayers() {
    int c = 0;
    for (const auto& p : players) {
        if (!p.isOut) c++;
    }
    return c > 0 ? c : 1;
}

void KozelEngine::advanceTurn() {
    if (gameOver) return;

    int n = players.size();
    for (int i = 1; i <= n; ++i) {
        int next = (currentTurnIdx + i) % n;
        if (!players[next].hand.isEmpty()) {
            currentTurnIdx = next;
            updateStatus();
            return;
        }
    }

    checkWinCondition();
}

void KozelEngine::updateStatus() {
    if (gameOver) return;
    if (currentTurnIdx == 0) {
        statusMessage = getLocalizedText("Ваш ход! Заходите любой картой.", "Your turn! Lead with any card.");
    } else {
        statusMessage = QString(getLocalizedText("Ход игрока %1", "%1's turn")).arg(players[currentTurnIdx].name);
    }
}

void KozelEngine::handlePlayerReconnect(int pIdx) {
    if (pIdx < 0 || pIdx >= players.size()) return;
    emit stateChanged();
}

void KozelEngine::handlePlayerDisconnect(int pIdx) {
    if (pIdx < 0 || pIdx >= players.size()) return;

    QString disconnectedName = players[pIdx].name;
    bool wasSpectator = players[pIdx].isOut;

    players.removeAt(pIdx);

    for (int i = 0; i < players.size(); ++i) {
        players[i].id = i;
    }

    if (leadPlayerIdx >= players.size()) leadPlayerIdx = 0;
    if (currentTurnIdx >= players.size()) currentTurnIdx = 0;

    if (wasSpectator) {
        emit stateChanged();
        return;
    }

    int activeCount = countActivePlayers();
    if (activeCount <= 1) {
        gameOver = true;
        statusMessage = getLocalizedText("Все оппоненты вышли! Игра завершена.", "All opponents left! Game over.");
        emit stateChanged();
        return;
    }

    statusMessage = QString(getLocalizedText("%1 вышел из игры! Игра продолжается.", "%1 left the game! Continuing.")).arg(disconnectedName);
    emit stateChanged();
}

QJsonObject KozelEngine::toJson(int targetId) const {
    QJsonObject json;
    json["yourId"]         = targetId;
    json["trumpSuit"]      = static_cast<int>(trumpSuit);
    json["leadPlayerIdx"]  = leadPlayerIdx;
    json["currentTurnIdx"] = currentTurnIdx;
    json["gameOver"]       = gameOver;
    json["statusMessage"]  = statusMessage;
    json["deckSize"]       = deck.size();

    QJsonArray trickArr;
    for (const auto& pair : currentTrick) {
        QJsonObject tObj;
        tObj["playerIdx"] = pair.first;
        tObj["card"]      = pair.second.toJson();
        trickArr.append(tObj);
    }
    json["currentTrick"] = trickArr;

    QJsonArray playersArr;
    for (const auto& p : players) {
        QJsonObject pObj;
        pObj["id"]              = p.id;
        pObj["name"]            = p.name;
        pObj["avatar"]          = p.avatar;
        pObj["pointsCollected"] = p.pointsCollected;
        pObj["isBot"]           = p.isBot;
        pObj["isOut"]           = p.isOut;
        pObj["handSize"]        = p.hand.size();

        QJsonArray handArr;
        if (p.id == targetId || gameOver) {
            for (const auto& c : p.hand) handArr.append(c.toJson());
        }
        pObj["hand"] = handArr;
        playersArr.append(pObj);
    }

    json["players"] = playersArr;
    return json;
}

void KozelEngine::fromJson(const QJsonObject& json) {
    if (json.contains("yourId")) {
        myIdx = json["yourId"].toInt();
    }
    trumpSuit      = static_cast<Suit>(json["trumpSuit"].toInt());
    leadPlayerIdx  = json["leadPlayerIdx"].toInt();
    currentTurnIdx = json["currentTurnIdx"].toInt();
    gameOver       = json["gameOver"].toBool();
    statusMessage  = json["statusMessage"].toString();

    int deckSize = json["deckSize"].toInt();
    deck.resize(deckSize);

    currentTrick.clear();
    for (auto val : json["currentTrick"].toArray()) {
        QJsonObject tObj = val.toObject();
        currentTrick.append({ tObj["playerIdx"].toInt(), Card::fromJson(tObj["card"].toObject()) });
    }

    QJsonArray pArr = json["players"].toArray();
    players.resize(pArr.size());
    for (int i = 0; i < pArr.size(); ++i) {
        QJsonObject pObj = pArr[i].toObject();
        players[i].id              = pObj["id"].toInt();
        players[i].name            = pObj["name"].toString();
        players[i].avatar          = pObj["avatar"].toInt(0);
        players[i].pointsCollected = pObj["pointsCollected"].toInt();
        players[i].isBot           = pObj["isBot"].toBool();
        players[i].isOut           = pObj["isOut"].toBool();

        int handSize = pObj["handSize"].toInt();
        players[i].hand.clear();
        QJsonArray handArr = pObj["hand"].toArray();

        if (!handArr.isEmpty()) {
            for (auto cVal : handArr) {
                players[i].hand.append(Card::fromJson(cVal.toObject()));
            }
        } else {
            players[i].hand.resize(handSize);
        }
    }
    emit stateChanged();
}
