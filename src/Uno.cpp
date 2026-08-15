#include "Uno.h"
#include "Audio.h"

void UnoEngine::createDeck() {
    deck.clear();
    for (int c = 0; c < 4; ++c) {
        UnoColor color = static_cast<UnoColor>(c);
        deck.append(UnoCard{ color, UnoZero });
        for (int i = 0; i < 2; ++i) {
            for (int v = UnoOne; v <= UnoNine; ++v)
                deck.append(UnoCard{ color, static_cast<UnoValue>(v) });
            deck.append(UnoCard{ color, UnoSkip });
            deck.append(UnoCard{ color, UnoReverse });
            deck.append(UnoCard{ color, UnoDrawTwo });
        }
    }
    for (int i = 0; i < 4; ++i) {
        deck.append(UnoCard{ UnoWild, UnoWildCard });
        deck.append(UnoCard{ UnoWild, UnoWildDrawFour });
    }
    for (int i = deck.size() - 1; i > 0; --i) {
        int j = QRandomGenerator::global()->bounded(i + 1);
        deck.swapItemsAt(i, j);
    }
}

void UnoEngine::replenishDeckIfNeeded() {
    if (deck.size() < 5 && discardPile.size() > 1) {
        UnoCard top = discardPile.takeLast();
        while (!discardPile.isEmpty()) {
            deck.append(discardPile.takeFirst());
        }
        for (int i = deck.size() - 1; i > 0; --i) {
            int j = QRandomGenerator::global()->bounded(i + 1);
            deck.swapItemsAt(i, j);
        }
        discardPile.append(top);
    }
}

void UnoEngine::initGame(int oppCount, bool netGame) {
    gameOver                 = false;
    isProcessingMove         = false;
    hasDrawnThisTurn         = false;
    accumulatedPenalty       = 0;
    direction                = 1;
    unoVulnerablePlayerIdx   = -1;
    unoVulnerabilityDeadline = 0;
    discardPile.clear();
    players.clear();
    myIdx                    = 0;

    drawMode        = AppSettings::instance().unoDrawMode;
    stackingEnabled = AppSettings::instance().unoStacking;

    UnoPlayer human;
    human.id = 0;
    human.name = AppSettings::instance().nickname;
    human.avatar = static_cast<int>(AppSettings::instance().avatar);
    human.isBot = false;
    human.saidUno = false;
    players.append(human);

    for (int i = 1; i <= oppCount; ++i) {
        UnoPlayer p;
        p.id = i;
        p.name = netGame ? QString(getLocalizedText("Игрок %1", "Player %1")).arg(i + 1) : QString(getLocalizedText("Бот %1", "Bot %1")).arg(i);
        p.avatar = netGame ? 0 : 4;
        p.isBot = !netGame;
        p.saidUno = false;
        players.append(p);
    }

    createDeck();

    for (auto& p : players) {
        p.hand.clear();
        p.saidUno = false;
        for (int c = 0; c < 7; ++c) {
            if (!deck.isEmpty()) p.hand.append(deck.takeFirst());
        }
    }

    // 1. ОФИЦИАЛЬНЫЕ ПРАВИЛА ПЕРВОЙ КАРТЫ СБРОСА
    while (!deck.isEmpty()) {
        UnoCard top = deck.takeFirst();
        if (top.value == UnoWildDrawFour) {
            deck.append(top); // Wild +4 нельзя открывать первым — замешиваем обратно
            continue;
        }
        discardPile.append(top);
        break;
    }

    UnoCard firstCard = discardPile.last();
    if (firstCard.color == UnoWild) {
        currentColor = static_cast<UnoColor>(QRandomGenerator::global()->bounded(4));
        currentTurnIdx = 0;
    } else {
        currentColor = firstCard.color;
        if (firstCard.value == UnoReverse) {
            direction = -1;
            currentTurnIdx = (players.size() == 2) ? 1 : (players.size() - 1);
        } else if (firstCard.value == UnoSkip) {
            currentTurnIdx = 1;
        } else if (firstCard.value == UnoDrawTwo) {
            if (stackingEnabled) {
                accumulatedPenalty = 2;
                currentTurnIdx = 0;
            } else {
                for (int i = 0; i < 2 && !deck.isEmpty(); ++i) players[0].hand.append(deck.takeFirst());
                currentTurnIdx = 1;
            }
        } else {
            currentTurnIdx = 0;
        }
    }

    updateStatus();
    emit stateChanged();
}

bool UnoEngine::canPlayCard(const UnoCard& card) const {
    if (discardPile.isEmpty()) return true;
    const UnoCard& top = discardPile.last();

    if (accumulatedPenalty > 0 && stackingEnabled) {
        if (top.value == UnoDrawTwo && card.value == UnoDrawTwo) return true;
        if (top.value == UnoWildDrawFour && card.value == UnoWildDrawFour) return true;
        return false;
    }

    if (card.color == UnoWild) return true;
    if (card.color == currentColor) return true;
    if (card.value == top.value) return true;
    return false;
}

bool UnoEngine::playCard(int playerIdx, int cardHandIdx, UnoColor chosenColor, bool callUno) {
    if (playerIdx != currentTurnIdx || gameOver) return false;
    auto& hand = players[playerIdx].hand;
    if (cardHandIdx < 0 || cardHandIdx >= hand.size()) return false;

    UnoCard card = hand[cardHandIdx];
    if (!canPlayCard(card)) return false;

    hand.removeAt(cardHandIdx);
    discardPile.append(card);
    AudioManager::instance().playSound(SoundEffect::CardPlace);

    // 2. ОБРАБОТКА ФОРЫ И ВЫКРИКА "УНО!"
    if (hand.size() == 1) {
        if (callUno) {
            players[playerIdx].saidUno = true;
            if (unoVulnerablePlayerIdx == playerIdx) {
                unoVulnerablePlayerIdx = -1;
                unoVulnerabilityDeadline = 0;
            }
        } else {
            players[playerIdx].saidUno = false;
            unoVulnerablePlayerIdx = playerIdx;
            unoVulnerabilityDeadline = QDateTime::currentMSecsSinceEpoch() + 3000; // Окно форы 3 секунды
        }
    } else {
        players[playerIdx].saidUno = false;
        if (unoVulnerablePlayerIdx == playerIdx) {
            unoVulnerablePlayerIdx = -1;
            unoVulnerabilityDeadline = 0;
        }
    }

    if (card.color == UnoWild) {
        currentColor = (chosenColor == UnoWild) ? UnoRed : chosenColor;
    } else {
        currentColor = card.color;
    }

    hasDrawnThisTurn = false;
    checkWinCondition();
    if (gameOver) return true;

    int n = players.size();
    if (card.value == UnoReverse) {
        direction = -direction;
        advanceTurn(n == 2 ? 2 : 1);
    } else if (card.value == UnoSkip) {
        advanceTurn(2);
    } else if (card.value == UnoDrawTwo) {
        if (stackingEnabled) {
            accumulatedPenalty += 2;
            advanceTurn(1);
        } else {
            int nextP = getNextActivePlayer(currentTurnIdx, 1);
            replenishDeckIfNeeded();
            for (int i = 0; i < 2 && !deck.isEmpty(); ++i) players[nextP].hand.append(deck.takeFirst());
            advanceTurn(2);
        }
    } else if (card.value == UnoWildDrawFour) {
        if (stackingEnabled) {
            accumulatedPenalty += 4;
            advanceTurn(1);
        } else {
            int nextP = getNextActivePlayer(currentTurnIdx, 1);
            replenishDeckIfNeeded();
            for (int i = 0; i < 4 && !deck.isEmpty(); ++i) players[nextP].hand.append(deck.takeFirst());
            advanceTurn(2);
        }
    } else {
        advanceTurn(1);
    }

    updateStatus();
    emit stateChanged();
    return true;
}

bool UnoEngine::checkStalemate() {
    if (!deck.isEmpty() || discardPile.size() > 1) return false;

    for (const auto& p : players) {
        if (p.isOut) continue;
        for (const auto& c : p.hand) {
            if (canPlayCard(c)) return false;
        }
    }

    gameOver = true;
    statusMessage = getLocalizedText("Тупик! Колода пуста и ходов нет. Ничья!",
                                     "Stalemate! Deck is empty and no valid moves. Draw!");
    AudioManager::instance().playSound(SoundEffect::Lose);
    emit stateChanged();
    return true;
}

bool UnoEngine::drawCard(int playerIdx) {
    if (playerIdx != currentTurnIdx || gameOver) return false;

    if (accumulatedPenalty > 0) {
        replenishDeckIfNeeded();
        for (int i = 0; i < accumulatedPenalty && !deck.isEmpty(); ++i) {
            players[playerIdx].hand.append(deck.takeFirst());
        }
        accumulatedPenalty = 0;
        players[playerIdx].saidUno = false;
        AudioManager::instance().playSound(SoundEffect::CardPlace);
        advanceTurn(1);
        updateStatus();
        emit stateChanged();
        return true;
    }

    if (drawMode == UnoDrawMode::DrawOne && hasDrawnThisTurn) return false;

    replenishDeckIfNeeded();
    if (!deck.isEmpty()) {
        players[playerIdx].hand.append(deck.takeFirst());
        players[playerIdx].saidUno = false;
        hasDrawnThisTurn = true;
        AudioManager::instance().playSound(SoundEffect::CardPlace);
        updateStatus();
        emit stateChanged();
        return true;
    }

    checkStalemate();
    return false;
}

void UnoEngine::passTurn(int playerIdx) {
    if (playerIdx != currentTurnIdx || !hasDrawnThisTurn || gameOver || accumulatedPenalty > 0) return;
    hasDrawnThisTurn = false;
    advanceTurn(1);
    updateStatus();
    emit stateChanged();
}

bool UnoEngine::declareUno(int playerIdx) {
    if (playerIdx < 0 || playerIdx >= players.size()) return false;
    if (players[playerIdx].hand.size() <= 2) {
        players[playerIdx].saidUno = true;
        if (unoVulnerablePlayerIdx == playerIdx) {
            unoVulnerablePlayerIdx = -1;
            unoVulnerabilityDeadline = 0;
        }
        AudioManager::instance().playSound(SoundEffect::Check);
        emit stateChanged();
        return true;
    }
    return false;
}

bool UnoEngine::catchUno(int catcherIdx, int targetIdx) {
    Q_UNUSED(catcherIdx);
    if (targetIdx < 0 || targetIdx >= players.size()) return false;
    auto& target = players[targetIdx];

    if (target.hand.size() == 1 && !target.saidUno) {
        replenishDeckIfNeeded();
        for (int i = 0; i < 2 && !deck.isEmpty(); ++i) {
            target.hand.append(deck.takeFirst());
        }
        target.saidUno = false;
        unoVulnerablePlayerIdx = -1;
        unoVulnerabilityDeadline = 0;
        AudioManager::instance().playSound(SoundEffect::Lose);
        statusMessage = QString(getLocalizedText("%1 пойман без УНО! (+2 карты штрафа)",
                                                 "%1 caught without UNO! (+2 penalty cards)")).arg(target.name);
        emit stateChanged();
        return true;
    }
    return false;
}

bool UnoEngine::hasPlayableCard(int playerIdx) const {
    if (playerIdx < 0 || playerIdx >= players.size()) return false;
    for (const auto& card : players[playerIdx].hand) {
        if (canPlayCard(card)) return true;
    }
    return false;
}

int UnoEngine::getNextActivePlayer(int current, int step) {
    int n = players.size();
    int next = current;
    for (int s = 0; s < step; ++s) {
        for (int i = 1; i <= n; ++i) {
            next = (next + direction + n) % n;
            if (!players[next].isOut) break;
        }
    }
    return next;
}

void UnoEngine::advanceTurn(int step) {
    currentTurnIdx = getNextActivePlayer(currentTurnIdx, step);
    hasDrawnThisTurn = false;
}

int UnoEngine::countActivePlayers() {
    int c = 0;
    for (const auto& p : players) if (!p.isOut) c++;
    return c;
}

void UnoEngine::checkWinCondition() {
    for (int i = 0; i < players.size(); ++i) {
        if (!players[i].isOut && players[i].hand.isEmpty()) {
            gameOver = true;
            AudioManager::instance().playSound(SoundEffect::Win);
            statusMessage = QString(getLocalizedText("ПОБЕДА! %1 выиграл!", "VICTORY! %1 won!")).arg(players[i].name);
            emit stateChanged();
            return;
        }
    }
}

bool UnoEngine::makeAiMove() {
    if (isProcessingMove || gameOver || currentTurnIdx >= players.size() || !players[currentTurnIdx].isBot) return false;

    // Честная ловля ботом: бот выжидает 1.5 - 2 секунды из 3-секундного окна форы
    if (unoVulnerablePlayerIdx >= 0 && unoVulnerablePlayerIdx != currentTurnIdx) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now >= unoVulnerabilityDeadline - 1200) {
            if (QRandomGenerator::global()->bounded(100) < 80) {
                catchUno(currentTurnIdx, unoVulnerablePlayerIdx);
                return true;
            }
        }
    }

    isProcessingMove = true;
    auto& hand = players[currentTurnIdx].hand;

    QVector<int> playable;
    for (int i = 0; i < hand.size(); ++i) {
        if (canPlayCard(hand[i])) playable.append(i);
    }

    if (!playable.isEmpty()) {
        int bestIdx = playable.first();
        for (int idx : playable) {
            if (hand[idx].color != UnoWild) { bestIdx = idx; break; }
        }

        UnoColor chosenColor = UnoRed;
        if (hand[bestIdx].color == UnoWild) {
            int counts[4] = {0, 0, 0, 0};
            for (const auto& c : hand) {
                if (c.color >= 0 && c.color < 4) counts[c.color]++;
            }
            int maxC = -1, bestColor = 0;
            for (int i = 0; i < 4; ++i) {
                if (counts[i] > maxC) { maxC = counts[i]; bestColor = i; }
            }
            chosenColor = static_cast<UnoColor>(bestColor);
        }

        bool callUno = (hand.size() == 2) && (QRandomGenerator::global()->bounded(100) < 85);
        playCard(currentTurnIdx, bestIdx, chosenColor, callUno);
        isProcessingMove = false;
        return true;
    }

    if (accumulatedPenalty > 0) {
        drawCard(currentTurnIdx);
        isProcessingMove = false;
        return true;
    }

    if (drawMode == UnoDrawMode::DrawUntilMatch) {
        drawCard(currentTurnIdx);
        if (!hand.isEmpty() && canPlayCard(hand.last())) {
            UnoColor chosenColor = UnoRed;
            if (hand.last().color == UnoWild) chosenColor = UnoBlue;
            bool callUno = (hand.size() == 2) && (QRandomGenerator::global()->bounded(100) < 85);
            playCard(currentTurnIdx, hand.size() - 1, chosenColor, callUno);
        }
        isProcessingMove = false;
        return true;
    } else {
        if (!hasDrawnThisTurn) {
            drawCard(currentTurnIdx);
            if (!hand.isEmpty() && canPlayCard(hand.last())) {
                UnoColor chosenColor = UnoRed;
                if (hand.last().color == UnoWild) chosenColor = UnoBlue;
                bool callUno = (hand.size() == 2) && (QRandomGenerator::global()->bounded(100) < 85);
                playCard(currentTurnIdx, hand.size() - 1, chosenColor, callUno);
            } else {
                passTurn(currentTurnIdx);
            }
            isProcessingMove = false;
            return true;
        } else {
            passTurn(currentTurnIdx);
            isProcessingMove = false;
            return true;
        }
    }
}

void UnoEngine::updateStatus() {
    if (gameOver) return;
    static const QString colNamesRu[] = { "Красный", "Жёлтый", "Зелёный", "Синий" };
    static const QString colNamesEn[] = { "Red", "Yellow", "Green", "Blue" };
    QString colorTxt = (QLocale::system().language() == QLocale::Russian) ? colNamesRu[currentColor] : colNamesEn[currentColor];

    QString penaltySuffix = (accumulatedPenalty > 0) ? QString(getLocalizedText(" [ШТРАФ: +%1]", " [PENALTY: +%1]")).arg(accumulatedPenalty) : "";

    if (currentTurnIdx == 0) {
        statusMessage = QString(getLocalizedText("Ваш ход! Цвет: %1%2", "Your turn! Color: %1%2")).arg(colorTxt, penaltySuffix);
    } else {
        statusMessage = QString(getLocalizedText("Ход игрока %1 (Цвет: %2)%3", "%1's turn (Color: %2)%3")).arg(players[currentTurnIdx].name, colorTxt, penaltySuffix);
    }
}

void UnoEngine::handlePlayerReconnect(int pIdx) {
    if (pIdx >= 0 && pIdx < players.size()) {
        players[pIdx].isOut = false;
        emit stateChanged();
    }
}

void UnoEngine::handlePlayerDisconnect(int pIdx) {
    if (pIdx < 0 || pIdx >= players.size()) return;
    QString discName = players[pIdx].name;
    players.removeAt(pIdx);
    for (int i = 0; i < players.size(); ++i) players[i].id = i;
    if (currentTurnIdx >= players.size()) currentTurnIdx = 0;
    if (countActivePlayers() <= 1) {
        gameOver = true;
        statusMessage = getLocalizedText("Все оппоненты вышли! Игра завершена.", "All opponents left! Game over.");
        emit stateChanged();
        return;
    }
    statusMessage = QString(getLocalizedText("%1 вышел! Игра продолжается.", "%1 left! Continuing.")).arg(discName);
    emit stateChanged();
}

QJsonObject UnoEngine::toJson(int targetId) const {
    QJsonObject json;
    json["yourId"]                   = targetId;
    json["currentColor"]             = static_cast<int>(currentColor);
    json["currentTurnIdx"]           = currentTurnIdx;
    json["direction"]                = direction;
    json["accumulatedPenalty"]       = accumulatedPenalty;
    json["hasDrawnThisTurn"]         = hasDrawnThisTurn;
    json["gameOver"]                 = gameOver;
    json["statusMessage"]            = statusMessage;
    json["deckSize"]                 = deck.size();

    // Синхронизация правил хоста
    json["drawMode"]                 = static_cast<int>(drawMode);
    json["stackingEnabled"]          = stackingEnabled;
    json["unoVulnerablePlayerIdx"]   = unoVulnerablePlayerIdx;
    json["unoVulnerabilityDeadline"] = unoVulnerabilityDeadline;

    // Синхронизация последних карт сброса для 3D-эффекта стопки
    QJsonArray discardArr;
    int startIdx = qMax(0, discardPile.size() - 5);
    for (int i = startIdx; i < discardPile.size(); ++i) {
        discardArr.append(discardPile[i].toJson());
    }
    json["discardPile"] = discardArr;

    if (!discardPile.isEmpty()) {
        json["topCard"] = discardPile.last().toJson();
    }

    QJsonArray playersArr;
    for (const auto& p : players) {
        QJsonObject pObj;
        pObj["id"]       = p.id;
        pObj["name"]     = p.name;
        pObj["avatar"]   = p.avatar;
        pObj["isBot"]    = p.isBot;
        pObj["isOut"]    = p.isOut;
        pObj["saidUno"]  = p.saidUno;
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

void UnoEngine::fromJson(const QJsonObject& json) {
    if (json.contains("yourId")) myIdx = json["yourId"].toInt();
    currentColor             = static_cast<UnoColor>(json["currentColor"].toInt());
    currentTurnIdx           = json["currentTurnIdx"].toInt();
    direction                = json["direction"].toInt(1);
    accumulatedPenalty       = json["accumulatedPenalty"].toInt(0);
    hasDrawnThisTurn         = json["hasDrawnThisTurn"].toBool();
    gameOver                 = json["gameOver"].toBool();
    statusMessage            = json["statusMessage"].toString();

    drawMode                 = static_cast<UnoDrawMode>(json["drawMode"].toInt(static_cast<int>(drawMode)));
    stackingEnabled          = json["stackingEnabled"].toBool(stackingEnabled);
    unoVulnerablePlayerIdx   = json["unoVulnerablePlayerIdx"].toInt(-1);
    unoVulnerabilityDeadline = json["unoVulnerabilityDeadline"].toVariant().toLongLong();

    deck.resize(json["deckSize"].toInt());
    discardPile.clear();

    if (json.contains("discardPile")) {
        for (auto v : json["discardPile"].toArray()) {
            discardPile.append(UnoCard::fromJson(v.toObject()));
        }
    } else if (json.contains("topCard")) {
        discardPile.append(UnoCard::fromJson(json["topCard"].toObject()));
    }

    QJsonArray pArr = json["players"].toArray();
    players.resize(pArr.size());
    for (int i = 0; i < pArr.size(); ++i) {
        QJsonObject pObj = pArr[i].toObject();
        players[i].id     = pObj["id"].toInt();
        players[i].name   = pObj["name"].toString();
        players[i].avatar = pObj["avatar"].toInt(0);
        players[i].isBot  = pObj["isBot"].toBool();
        players[i].isOut  = pObj["isOut"].toBool();
        players[i].saidUno= pObj["saidUno"].toBool(false);

        int handSize = pObj["handSize"].toInt();
        players[i].hand.clear();
        QJsonArray handArr = pObj["hand"].toArray();
        if (!handArr.isEmpty()) {
            for (auto v : handArr) players[i].hand.append(UnoCard::fromJson(v.toObject()));
        } else {
            players[i].hand.resize(handSize);
        }
    }
    emit stateChanged();
}
