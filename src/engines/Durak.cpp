#include "Durak.h"
#include "Audio.h"
#include "AppSettings.h"

void DurakEngine::initGame(int oppCount, bool netGame) {
    gameOver = false;
    isDefenderTaking = false;
    isProcessingMove = false;
    table.clear();
    deck.clear();
    players.clear();
    bitoCount = 0;
    myIdx = 0;

    DurakPlayer human;
    human.id = 0;
    human.name = AppSettings::instance().getNickname();
    human.avatar = static_cast<int>(AppSettings::instance().getAvatar());
    human.isBot = false;
    players.append(human);

    for (int i = 1; i <= oppCount; ++i) {
        DurakPlayer p;
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
        const int j = QRandomGenerator::global()->bounded(i + 1);
        deck.swapItemsAt(i, j);
    }

    trumpCard = deck.last();
    replenishHands();

    int minTrump = 99;
    attackerIdx = 0;
    for (int i = 0; i < players.size(); ++i) {
        for (const auto& c : players[i].hand) {
            if (c.suit == trumpCard.suit && c.rank < minTrump) {
                minTrump = c.rank;
                attackerIdx = i;
            }
        }
    }
    defenderIdx = (attackerIdx + 1) % players.size();
    currentTurnIdx = attackerIdx;
    statusMessage = (attackerIdx == 0) ? getLocalizedText("Ваш ход! Атакуйте!", "Your turn! Attack!") : QString(getLocalizedText("Ход игрока %1", "%1's turn")).arg(players[attackerIdx].name);
    emit stateChanged();
}

void DurakEngine::replenishHands() {
    if (deck.isEmpty()) return;
    const int n = players.size();

    for (int i = 0; i < n; ++i) {
        const int pIdx = (attackerIdx + i) % n;
        if (pIdx == defenderIdx) continue;

        if (!players[pIdx].isOut) {
            while (players[pIdx].hand.size() < 6 && !deck.isEmpty()) {
                players[pIdx].hand.append(deck.takeFirst());
            }
        }
    }

    if (defenderIdx >= 0 && defenderIdx < n && !players[defenderIdx].isOut) {
        while (players[defenderIdx].hand.size() < 6 && !deck.isEmpty()) {
            players[defenderIdx].hand.append(deck.takeFirst());
        }
    }
}

bool DurakEngine::canAttackWith(const Card& card) const {
    if (table.isEmpty()) return true;
    for (const auto& pair : table) {
        if (pair.attack.rank == card.rank) return true;
        if (pair.isDefended && pair.defend.rank == card.rank) return true;
    }
    return false;
}

bool DurakEngine::playAttackCard(int playerIdx, int cardHandIdx) {
    if (playerIdx < 0 || playerIdx >= players.size()) return false;
    if (playerIdx == defenderIdx && isDefenderTaking) return false;

    auto& hand = players[playerIdx].hand;
    if (cardHandIdx < 0 || cardHandIdx >= hand.size()) return false;

    const Card card = hand[cardHandIdx];
    if (!canAttackWith(card)) return false;

    const int defenderCardsCount = players[defenderIdx].hand.size();
    int undefendedCount = 0;
    for (const auto& p : table) if (!p.isDefended) undefendedCount++;
    if (undefendedCount >= defenderCardsCount || table.size() >= 6) return false;

    hand.removeAt(cardHandIdx);
    table.append(DurakTablePair{ card, Card{}, false });

    if (isDefenderTaking) {
        int newUndefended = 0;
        for (const auto& p : table) if (!p.isDefended) newUndefended++;

        if (newUndefended >= defenderCardsCount || table.size() >= 6) {
            passAction();
            return true;
        }
    } else {
        currentTurnIdx = defenderIdx;
    }

    emit stateChanged();
    AudioManager::instance().playSound(SoundEffect::CardPlace);
    return true;
}

bool DurakEngine::playDefendCard(int playerIdx, int cardHandIdx, int tableIdx) {
    if (playerIdx != defenderIdx) return false;
    auto& hand = players[playerIdx].hand;
    if (cardHandIdx < 0 || cardHandIdx >= hand.size()) return false;
    if (tableIdx < 0 || tableIdx >= table.size()) return false;
    if (table[tableIdx].isDefended) return false;

    const Card defCard = hand[cardHandIdx];
    const Card atkCard = table[tableIdx].attack;

    bool beats = false;
    if (defCard.suit == atkCard.suit && defCard.rank > atkCard.rank) beats = true;
    else if (defCard.suit == trumpCard.suit && atkCard.suit != trumpCard.suit) beats = true;

    if (beats) {
        hand.removeAt(cardHandIdx);
        table[tableIdx].defend = defCard;
        table[tableIdx].isDefended = true;

        currentTurnIdx = attackerIdx;

        bool allDefended = true;
        for (const auto& pair : table) if (!pair.isDefended) allDefended = false;

        if (allDefended && (players[attackerIdx].hand.isEmpty() || players[defenderIdx].hand.isEmpty())) {
            passAction();
            return true;
        }

        emit stateChanged();
        AudioManager::instance().playSound(SoundEffect::CardPlace);
        return true;
    }
    return false;
}

void DurakEngine::passAction() {
    if (table.isEmpty()) return;

    if (isDefenderTaking) {
        AudioManager::instance().playSound(SoundEffect::Lose);

        for (const auto& p : table) {
            players[defenderIdx].hand.append(p.attack);
            if (p.isDefended) players[defenderIdx].hand.append(p.defend);
        }
        table.clear();
        replenishHands();
        isDefenderTaking = false;

        checkWinCondition();
        if (gameOver) return;

        attackerIdx = getNextActivePlayer(defenderIdx);
        defenderIdx = getNextActivePlayer(attackerIdx);
        currentTurnIdx = attackerIdx;
    } else {
        AudioManager::instance().playSound(SoundEffect::Bito);

        bool allDefended = true;
        for (const auto& p : table) if (!p.isDefended) allDefended = false;
        if (!allDefended) return;

        bitoCount += table.size() * 2;
        table.clear();
        replenishHands();

        checkWinCondition();
        if (gameOver) return;

        if (!players[defenderIdx].isOut && !players[defenderIdx].hand.isEmpty()) {
            attackerIdx = defenderIdx;
        } else {
            attackerIdx = getNextActivePlayer(defenderIdx);
        }
        defenderIdx = getNextActivePlayer(attackerIdx);
        currentTurnIdx = attackerIdx;
    }

    if (attackerIdx == defenderIdx && countActivePlayers() <= 1) {
        checkWinCondition();
        return;
    }

    updateStatus();
    emit stateChanged();
}

void DurakEngine::takeAction() {
    if (table.isEmpty()) return;

    const int defenderCardsCount = players[defenderIdx].hand.size();
    int undefendedCount = 0;
    for (const auto& p : table) if (!p.isDefended) undefendedCount++;

    isDefenderTaking = true;

    if (undefendedCount >= defenderCardsCount || table.size() >= 6) {
        passAction();
        return;
    }

    int firstTosser = -1;
    const int n = players.size();
    for (int i = 0; i < n; ++i) {
        const int pIdx = (attackerIdx + i) % n;
        if (pIdx != defenderIdx && !players[pIdx].isOut && !players[pIdx].hand.isEmpty()) {
            firstTosser = pIdx;
            break;
        }
    }

    if (firstTosser == -1) {
        passAction();
        return;
    }

    currentTurnIdx = firstTosser;
    updateStatus();
    emit stateChanged();
}

void DurakEngine::checkWinCondition() {
    if (deck.isEmpty()) {
        for (auto& p : players) {
            if (!p.isOut && p.hand.isEmpty()) {
                p.isOut = true;
            }
        }
    }

    if (countActivePlayers() <= 1) {
        gameOver = true;
        AudioManager::instance().playSound(SoundEffect::Win);

        int durakIdx = -1;
        for (int i = 0; i < players.size(); ++i) {
            if (!players[i].isOut && !players[i].hand.isEmpty()) {
                durakIdx = i;
                break;
            }
        }

        if (durakIdx != -1) {
            statusMessage = QString(getLocalizedText("Игра окончена! %1 — ДУРАК!", "Game over! %1 is the DURAK!")).arg(players[durakIdx].name);
        } else {
            statusMessage = getLocalizedText("Игра окончена! Ничья!", "Game over! Draw!");
        }
        emit stateChanged();
    }
}

bool DurakEngine::makeAiMove() {
    if (isProcessingMove || gameOver) return false;

    isProcessingMove = true;

    if (table.isEmpty()) {
        while (players[attackerIdx].isOut || players[attackerIdx].hand.isEmpty()) {
            const int nextAttacker = getNextActivePlayer(attackerIdx);
            if (nextAttacker == attackerIdx) {
                checkWinCondition();
                isProcessingMove = false;
                return false;
            }
            attackerIdx = nextAttacker;
            defenderIdx = getNextActivePlayer(attackerIdx);
            currentTurnIdx = attackerIdx;
        }
        while (players[defenderIdx].isOut || players[defenderIdx].hand.isEmpty()) {
            defenderIdx = getNextActivePlayer(defenderIdx);
            if (defenderIdx == attackerIdx) {
                checkWinCondition();
                isProcessingMove = false;
                return false;
            }
        }
    }

    if (isDefenderTaking) {
        auto getNextTosser = [this](int current) {
            const int n = players.size();
            for (int i = 1; i <= n; ++i) {
                const int next = (current + i) % n;
                if (next != defenderIdx && !players[next].isOut && !players[next].hand.isEmpty()) {
                    return next;
                }
            }
            return -1;
        };

        const int activeTosser = getNextTosser(currentTurnIdx);

        if (activeTosser == -1) {
            passAction();
            isProcessingMove = false;
            return true;
        }

        if (currentTurnIdx == defenderIdx || players[currentTurnIdx].isOut || players[currentTurnIdx].hand.isEmpty()) {
            currentTurnIdx = activeTosser;
            updateStatus();
            emit stateChanged();
            isProcessingMove = false;
            return true;
        }

        if (!players[currentTurnIdx].isBot && !players[currentTurnIdx].isOut) {
            updateStatus();
            isProcessingMove = false;
            return false;
        }

        if (players[currentTurnIdx].isBot && !players[currentTurnIdx].isOut) {
            auto& hand = players[currentTurnIdx].hand;
            bool threwIn = false;

            for (int i = 0; i < hand.size(); ++i) {
                if (canAttackWith(hand[i])) {
                    if (playAttackCard(currentTurnIdx, i)) {
                        threwIn = true;
                        break;
                    }
                }
            }

            if (!threwIn) {
                const int nextP = getNextTosser(currentTurnIdx);
                if (nextP == -1 || nextP == currentTurnIdx) {
                    passAction();
                } else {
                    currentTurnIdx = nextP;
                    updateStatus();
                    emit stateChanged();
                }
            }
            isProcessingMove = false;
            return true;
        }
        isProcessingMove = false;
        return false;
    }

    if (players[attackerIdx].isBot && !players[attackerIdx].isOut) {
        auto& hand = players[attackerIdx].hand;

        if (table.isEmpty()) {
            if (hand.isEmpty()) {
                isProcessingMove = false;
                return false;
            }

            int bestCardIdx = -1;
            int minRankNonTrump = 99;
            int minRankTrump = 99;
            int bestTrumpIdx = -1;

            for (int i = 0; i < hand.size(); ++i) {
                if (hand[i].suit != trumpCard.suit) {
                    if (hand[i].rank < minRankNonTrump) {
                        minRankNonTrump = hand[i].rank;
                        bestCardIdx = i;
                    }
                } else {
                    if (hand[i].rank < minRankTrump) {
                        minRankTrump = hand[i].rank;
                        bestTrumpIdx = i;
                    }
                }
            }

            const int playIdx = (bestCardIdx != -1) ? bestCardIdx : bestTrumpIdx;
            if (playIdx != -1) {
                playAttackCard(attackerIdx, playIdx);
                isProcessingMove = false;
                return true;
            }
        } else {
            bool allDefended = true;
            for (const auto& p : table) if (!p.isDefended) allDefended = false;

            if (allDefended) {
                if (table.size() >= 6 || hand.isEmpty() || players[defenderIdx].hand.isEmpty()) {
                    passAction();
                    isProcessingMove = false;
                    return true;
                }

                int bestThrowIdx = -1;
                int minRank = 99;
                for (int i = 0; i < hand.size(); ++i) {
                    if (canAttackWith(hand[i])) {
                        if (hand[i].suit == trumpCard.suit && hand[i].rank >= 12) continue;
                        if (hand[i].rank < minRank) {
                            minRank = hand[i].rank;
                            bestThrowIdx = i;
                        }
                    }
                }

                if (bestThrowIdx != -1) {
                    if (playAttackCard(attackerIdx, bestThrowIdx)) {
                        isProcessingMove = false;
                        return true;
                    }
                }

                passAction();
                isProcessingMove = false;
                return true;
            }
        }
    }

    if (players[defenderIdx].isBot && !players[defenderIdx].isOut) {
        auto& hand = players[defenderIdx].hand;

        QVector<int> undefendedTableIndices;
        for (int t = 0; t < table.size(); ++t) {
            if (!table[t].isDefended) undefendedTableIndices.append(t);
        }

        if (!undefendedTableIndices.isEmpty()) {
            QVector<Card> tempHand = hand;
            bool canDefendAll = true;

            for (int tIdx : undefendedTableIndices) {
                const Card atk = table[tIdx].attack;
                int bestDefIdx = -1;
                int minWeight = 999;

                for (int c = 0; c < tempHand.size(); ++c) {
                    const Card def = tempHand[c];
                    const bool beats = (def.suit == atk.suit && def.rank > atk.rank) ||
                    (def.suit == trumpCard.suit && atk.suit != trumpCard.suit);

                    if (beats) {
                        const int weight = def.rank + (def.suit == trumpCard.suit ? 100 : 0);
                        if (weight < minWeight) {
                            minWeight = weight;
                            bestDefIdx = c;
                        }
                    }
                }

                if (bestDefIdx != -1) {
                    tempHand.removeAt(bestDefIdx);
                } else {
                    canDefendAll = false;
                    break;
                }
            }

            if (!canDefendAll) {
                takeAction();
                isProcessingMove = false;
                return true;
            }

            const int tIdx = undefendedTableIndices.first();
            const Card atk = table[tIdx].attack;
            int bestHandIdx = -1;
            int minWeight = 999;

            for (int c = 0; c < hand.size(); ++c) {
                const Card def = hand[c];
                const bool beats = (def.suit == atk.suit && def.rank > atk.rank) ||
                (def.suit == trumpCard.suit && atk.suit != trumpCard.suit);

                if (beats) {
                    const int weight = def.rank + (def.suit == trumpCard.suit ? 100 : 0);
                    if (weight < minWeight) {
                        minWeight = weight;
                        bestHandIdx = c;
                    }
                }
            }

            if (bestHandIdx != -1) {
                playDefendCard(defenderIdx, bestHandIdx, tIdx);
                isProcessingMove = false;
                return true;
            } else {
                takeAction();
                isProcessingMove = false;
                return true;
            }
        }
    }

    isProcessingMove = false;
    return false;
}

int DurakEngine::countActivePlayers() {
    int c = 0;
    for (const auto& p : players) {
        if (!p.isOut && (!p.hand.isEmpty() || !deck.isEmpty())) c++;
    }
    return c;
}

int DurakEngine::getNextActivePlayer(int current) {
    const int n = players.size();
    for (int i = 1; i <= n; ++i) {
        const int next = (current + i) % n;
        if (!players[next].isOut && !players[next].hand.isEmpty()) {
            return next;
        }
    }
    return current;
}

void DurakEngine::updateStatus() {
    if (gameOver) return;

    if (isDefenderTaking) {
        statusMessage = QString(getLocalizedText("%1 берет карты!", "%1 takes cards!")).arg(players[defenderIdx].name);
        return;
    }

    statusMessage = QString(getLocalizedText("Ход игрока %1", "%1's turn")).arg(players[attackerIdx].name);
}

void DurakEngine::handlePlayerReconnect(int pIdx) {
    if (pIdx < 0 || pIdx >= players.size()) return;
    players[pIdx].isOut = false;
    emit stateChanged();
}

void DurakEngine::handlePlayerDisconnect(int pIdx) {
    if (pIdx < 0 || pIdx >= players.size()) return;

    const QString disconnectedName = players[pIdx].name;
    const bool wasSpectator = players[pIdx].isOut && !deck.isEmpty();

    players.removeAt(pIdx);

    for (int i = 0; i < players.size(); ++i) {
        players[i].id = i;
    }

    if (attackerIdx >= players.size()) attackerIdx = 0;
    if (defenderIdx >= players.size()) defenderIdx = (attackerIdx + 1) % std::max<int>(1, players.size());
    if (currentTurnIdx >= players.size()) currentTurnIdx = attackerIdx;

    if (wasSpectator) {
        emit stateChanged();
        return;
    }

    const int active = countActivePlayers();
    if (active <= 1) {
        gameOver = true;
        statusMessage = getLocalizedText("Все оппоненты вышли! Игра завершена.", "All opponents left! Game over.");
        emit stateChanged();
        return;
    }

    statusMessage = QString(getLocalizedText("%1 вышел из игры! Игра продолжается.", "%1 left the game! Continuing.")).arg(disconnectedName);
    emit stateChanged();
}

QJsonObject DurakEngine::toJson(int targetId) const {
    QJsonObject json;
    json["yourId"]           = targetId;
    json["attackerIdx"]      = attackerIdx;
    json["defenderIdx"]      = defenderIdx;
    json["currentTurnIdx"]   = currentTurnIdx;
    json["isDefenderTaking"] = isDefenderTaking;
    json["bitoCount"]        = bitoCount;
    json["gameOver"]         = gameOver;
    json["statusMessage"]    = statusMessage;
    json["deckSize"]         = deck.size();
    json["trumpCard"]        = trumpCard.toJson();

    QJsonArray tableArr;
    for (const auto& pair : table) {
        QJsonObject pObj;
        pObj["attack"]     = pair.attack.toJson();
        pObj["defend"]     = pair.defend.toJson();
        pObj["isDefended"] = pair.isDefended;
        tableArr.append(pObj);
    }
    json["table"] = tableArr;

    QJsonArray playersArr;
    for (const auto& p : players) {
        QJsonObject pObj;
        pObj["id"]       = p.id;
        pObj["name"]     = p.name;
        pObj["avatar"]   = p.avatar;
        pObj["isBot"]    = p.isBot;
        pObj["isOut"]    = p.isOut;
        pObj["handSize"] = p.hand.size();

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

void DurakEngine::fromJson(const QJsonObject& json) {
    if (json.contains("yourId")) {
        myIdx = json["yourId"].toInt();
    }
    attackerIdx      = json["attackerIdx"].toInt();
    defenderIdx      = json["defenderIdx"].toInt();
    currentTurnIdx   = json["currentTurnIdx"].toInt();
    isDefenderTaking = json["isDefenderTaking"].toBool();
    bitoCount        = json["bitoCount"].toInt();
    gameOver         = json["gameOver"].toBool();
    statusMessage    = json["statusMessage"].toString();
    trumpCard        = Card::fromJson(json["trumpCard"].toObject());

    const int deckSize = json["deckSize"].toInt();
    deck.resize(deckSize);

    table.clear();
    for (auto val : json["table"].toArray()) {
        const QJsonObject pObj = val.toObject();
        DurakTablePair pair;
        pair.attack     = Card::fromJson(pObj["attack"].toObject());
        pair.defend     = Card::fromJson(pObj["defend"].toObject());
        pair.isDefended = pObj["isDefended"].toBool();
        table.append(pair);
    }

    const QJsonArray pArr = json["players"].toArray();
    players.resize(pArr.size());
    for (int i = 0; i < pArr.size(); ++i) {
        const QJsonObject pObj = pArr[i].toObject();
        players[i].id     = pObj["id"].toInt();
        players[i].name   = pObj["name"].toString();
        players[i].avatar = pObj["avatar"].toInt(0);
        players[i].isBot  = pObj["isBot"].toBool();
        players[i].isOut  = pObj["isOut"].toBool();

        const int handSize = pObj["handSize"].toInt();
        players[i].hand.clear();
        const QJsonArray handArr = pObj["hand"].toArray();

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
