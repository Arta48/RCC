#include "Poker.h"
#include "Audio.h"
#include "AppSettings.h"

QJsonObject Card::toJson() const {
    QJsonObject obj;
    obj["suit"] = static_cast<int>(suit);
    obj["rank"] = rank;
    return obj;
}

Card Card::fromJson(const QJsonObject& obj) {
    return Card{ static_cast<Suit>(obj["suit"].toInt(0)), obj["rank"].toInt(2) };
}

bool Card::operator<(const Card& o) const { return rank < o.rank; }
bool Card::operator==(const Card& o) const { return rank == o.rank && suit == o.suit; }

HandValue evaluate5Cards(QVector<Card> c) {
    std::sort(c.rbegin(), c.rend());

    bool isFlush = (c[0].suit == c[1].suit && c[1].suit == c[2].suit &&
    c[2].suit == c[3].suit && c[3].suit == c[4].suit);

    bool isStraight = false;
    int straightHigh = c[0].rank;

    if (c[0].rank == c[1].rank + 1 && c[1].rank == c[2].rank + 1 &&
        c[2].rank == c[3].rank + 1 && c[3].rank == c[4].rank + 1) {
        isStraight = true;
    straightHigh = c[0].rank;
        } else if (c[0].rank == 14 && c[1].rank == 5 && c[2].rank == 4 &&
            c[3].rank == 3 && c[4].rank == 2) {
            isStraight = true;
        straightHigh = 5;
            }

            std::map<int, int> counts;
            for (const auto& card : c) counts[card.rank]++;

            int quad = 0, trip = 0;
    QVector<int> pairs;
    QVector<int> singleKickers;

    for (int r = 14; r >= 2; --r) {
        if (counts[r] == 4) quad = r;
        else if (counts[r] == 3) trip = r;
        else if (counts[r] == 2) pairs.append(r);
        else if (counts[r] == 1) singleKickers.append(r);
    }

    if (isStraight && isFlush) return {StraightFlush, (8u << 28) | (straightHigh << 24), getLocalizedText("Стрит-Флеш", "Straight Flush")};
    if (quad > 0) return {FourOfAKind, (7u << 28) | (quad << 24) | ((singleKickers.isEmpty() ? 0 : singleKickers[0]) << 20), getLocalizedText("Каре", "Four of a Kind")};
    if (trip > 0 && !pairs.isEmpty()) return {FullHouse, (6u << 28) | (trip << 24) | (pairs[0] << 20), getLocalizedText("Фулл-Хаус", "Full House")};
    if (isFlush) return {Flush, (5u << 28) | (c[0].rank << 16) | (c[1].rank << 12) | (c[2].rank << 8) | (c[3].rank << 4) | c[4].rank, getLocalizedText("Флеш", "Flush")};
    if (isStraight) return {Straight, (4u << 28) | (straightHigh << 24), getLocalizedText("Стрит", "Straight")};
    if (trip > 0) return {ThreeOfAKind, (3u << 28) | (trip << 24) | ((!singleKickers.isEmpty() ? singleKickers[0] : 0) << 20) | ((singleKickers.size() > 1 ? singleKickers[1] : 0) << 16), getLocalizedText("Сет (Тройка)", "Three of a Kind")};
    if (pairs.size() >= 2) return {TwoPair, (2u << 28) | (pairs[0] << 24) | (pairs[1] << 20) | ((!singleKickers.isEmpty() ? singleKickers[0] : 0) << 16), getLocalizedText("Две Пары", "Two Pair")};
    if (pairs.size() == 1) return {Pair, (1u << 28) | (pairs[0] << 24) | ((!singleKickers.isEmpty() ? singleKickers[0] : 0) << 20) | ((singleKickers.size() > 1 ? singleKickers[1] : 0) << 16) | ((singleKickers.size() > 2 ? singleKickers[2] : 0) << 12), getLocalizedText("Пара", "Pair")};

    return {HighCard, (0u << 28) | (c[0].rank << 16) | (c[1].rank << 12) | (c[2].rank << 8) | (c[3].rank << 4) | c[4].rank, getLocalizedText("Старшая Карта", "High Card")};
}

HandValue evaluate7Cards(const QVector<Card>& cards) {
    if (cards.size() < 5) return {HighCard, 0, ""};
    if (cards.size() == 5) return evaluate5Cards(cards);

    HandValue best = {HighCard, 0, ""};

    // Оптимизированный перебор ровно 21 сочетания
    if (cards.size() == 7) {
        for (int i = 0; i < 7; ++i) {
            for (int j = i + 1; j < 7; ++j) {
                QVector<Card> combo;
                combo.reserve(5);
                for (int k = 0; k < 7; ++k) {
                    if (k != i && k != j) combo.append(cards[k]);
                }
                HandValue hv = evaluate5Cards(combo);
                if (hv.score > best.score) best = hv;
            }
        }
        return best;
    }

    for (int i = 0; i < (1 << cards.size()); i++) {
        QVector<Card> combo;
        for (int j = 0; j < cards.size(); j++) {
            if (i & (1 << j)) combo.push_back(cards[j]);
        }
        if (combo.size() == 5) {
            HandValue hv = evaluate5Cards(combo);
            if (hv.score > best.score) best = hv;
        }
    }
    return best;
}

PokerEngine::PokerEngine(QObject* parent) : QObject(parent) {}

void PokerEngine::initGame(int oppCount, bool netGame) {
    gameOver = false;
    isProcessingMove = false;
    myIdx = 0;

    QVector<QPair<QString, int>> existingPlayerInfo;
    for (const auto& p : players) {
        existingPlayerInfo.append({ p.name, p.avatar });
    }

    players.clear();

    Player human;
    human.id = 0;
    human.name = AppSettings::instance().nickname;
    human.avatar = static_cast<int>(AppSettings::instance().avatar);
    human.isBot = false;
    players.append(human);

    for (int i = 1; i <= oppCount; ++i) {
        Player p;
        p.id = i;
        if (netGame && i < existingPlayerInfo.size() && !existingPlayerInfo[i].first.isEmpty()) {
            p.name = existingPlayerInfo[i].first;
            p.avatar = existingPlayerInfo[i].second;
        } else {
            p.name = netGame ? QString(getLocalizedText("Игрок %1", "Player %1")).arg(i + 1) : QString(getLocalizedText("Бот %1", "Bot %1")).arg(i);
            p.avatar = netGame ? 0 : 4;
        }
        p.isBot = !netGame;
        players.append(p);
    }
    dealerIdx = players.size() - 1;
    startNewHand();
}

int PokerEngine::countSolventPlayers() {
    int c = 0;
    for (const auto& p : players) if (p.balance > 0 && !p.isDisconnected) c++;
    return c;
}

void PokerEngine::resetGame() {
    for (auto& p : players) {
        if (!p.isDisconnected) {
            p.balance = PokerConfig::DEFAULT_BALANCE;
            p.isBankrupt = false;
        }
    }
    dealerIdx = players.size() - 1;
    startNewHand();
}

void PokerEngine::startNewHand() {
    for (auto& p : players) {
        if (p.isDisconnected) p.hasFolded = true;
        if (p.balance <= 0) p.isBankrupt = true;
        else if (!p.isDisconnected) p.isBankrupt = false;
    }

    bool isSinglePlayer = true;
    for (int i = 1; i < players.size(); ++i) {
        if (!players[i].isBot) { isSinglePlayer = false; break; }
    }

    if ((isSinglePlayer && myIdx < players.size() && players[myIdx].isBankrupt) || countSolventPlayers() < 2) {
        gameOver = true;
        if (myIdx < players.size() && players[myIdx].balance <= 0) {
            statusMessage = getLocalizedText("ВЫ ПРОИГРАЛИ! Ваш баланс $0.", "YOU LOST! Your balance is $0.");
        } else {
            statusMessage = getLocalizedText("ИГРА ОКОНЧЕНА! Все оппоненты вышли или разорены.", "GAME OVER! All opponents left or broke.");
        }
        emit stateChanged();
        return;
    }

    deck.clear();
    for (int s = 0; s < 4; ++s) {
        for (int r = 2; r <= 14; ++r) {
            deck.append(Card{ static_cast<Suit>(s), r });
        }
    }

    AudioManager::instance().playSound(SoundEffect::CardShuffle);

    for (int i = deck.size() - 1; i > 0; --i) {
        int j = QRandomGenerator::global()->bounded(i + 1);
        deck.swapItemsAt(i, j);
    }

    communityCards.clear();
    pot = 0;
    currentHighestBet = 0;
    minRaise = bigBlind;
    playersActed = 0;
    gameOver = false;
    phase = PREFLOP;

    for (auto& p : players) {
        p.holeCards.clear();
        p.holeCards.append(deck.takeFirst());
        p.holeCards.append(deck.takeFirst());
        p.hasFolded = p.isBankrupt || p.isDisconnected;
        p.isAllIn = false;
        p.currentBet = 0;
        p.totalContributed = 0;
        p.bestHand = {HighCard, 0, ""};
    }

    dealerIdx = getNextActivePlayer(dealerIdx);

    int sbIdx = getNextActivePlayer(dealerIdx);
    int bbIdx = getNextActivePlayer(sbIdx);
    if (countSolventPlayers() == 2) {
        sbIdx = dealerIdx;
        bbIdx = getNextActivePlayer(dealerIdx);
    }

    placeBet(sbIdx, std::min(smallBlind, players[sbIdx].balance));
    placeBet(bbIdx, std::min(bigBlind, players[bbIdx].balance));

    currentTurnIdx = getNextActivePlayer(bbIdx);
    updateStatus();
    emit stateChanged();
}

int PokerEngine::getNextActivePlayer(int current) {
    for (int i = 1; i <= players.size(); ++i) {
        int idx = (current + i) % players.size();
        if (!players[idx].hasFolded && !players[idx].isAllIn &&
            !players[idx].isBankrupt && !players[idx].isDisconnected) return idx;
    }
    return current;
}

int PokerEngine::countActivePlayers() {
    int c = 0;
    for (const auto& p : players) if (!p.hasFolded) c++;
    return c;
}

int PokerEngine::countNonAllInPlayers() {
    int c = 0;
    for (const auto& p : players) if (!p.hasFolded && !p.isAllIn) c++;
    return c;
}

void PokerEngine::placeBet(int pIdx, int amount) {
    players[pIdx].balance -= amount;
    players[pIdx].currentBet += amount;
    players[pIdx].totalContributed += amount;
    pot += amount;
    if (players[pIdx].currentBet > currentHighestBet) {
        int raiseDiff = players[pIdx].currentBet - currentHighestBet;
        if (raiseDiff > minRaise) minRaise = raiseDiff;
        currentHighestBet = players[pIdx].currentBet;
    }
    if (players[pIdx].balance == 0) players[pIdx].isAllIn = true;
}

void PokerEngine::handlePlayerDisconnect(int pIdx) {
    if (pIdx < 0 || pIdx >= players.size()) return;

    players[pIdx].isDisconnected = true;
    players[pIdx].hasFolded = true;

    if (gameOver) return;

    int activeCount = countActivePlayers();
    QString disconnectedName = players[pIdx].name.isEmpty() ? QString(getLocalizedText("Игрок %1", "Player %1")).arg(pIdx + 1) : players[pIdx].name;

    if (activeCount <= 1) {
        gameOver = true;
        bool winnerFound = false;
        for (auto& plr : players) {
            if (!plr.hasFolded) {
                plr.balance += pot;
                statusMessage = QString(getLocalizedText("%1 вышел. %2 забирает банк $%3!", "%1 left. %2 takes the pot of $%3!")).arg(disconnectedName, plr.name).arg(pot);
                winnerFound = true;
                break;
            }
        }
        if (!winnerFound) {
            statusMessage = QString(getLocalizedText("%1 вышел! Все остальные игроки также отключились.", "%1 left! All other players disconnected.")).arg(disconnectedName);
        }
    } else {
        statusMessage = QString(getLocalizedText("%1 вышел из игры! Игра продолжается.", "%1 left the game! Continuing.")).arg(disconnectedName);
        if (currentTurnIdx == pIdx) {
            currentTurnIdx = getNextActivePlayer(currentTurnIdx);
        }
        checkPhaseAdvance();
    }

    emit stateChanged();
}

void PokerEngine::handlePlayerReconnect(int pIdx) {
    if (pIdx < 0 || pIdx >= players.size()) return;
    players[pIdx].isDisconnected = false;
    emit stateChanged();
}

void PokerEngine::processAction(int pIdx, const QString& action, int raiseTotal) {
    if (pIdx < 0 || pIdx >= players.size()) return;
    if (currentTurnIdx < 0 || currentTurnIdx >= players.size()) return;
    if (pIdx != currentTurnIdx || phase == SHOWDOWN || gameOver) return;

    Player& p = players[pIdx];
    if (p.hasFolded || p.isBankrupt || p.isDisconnected) return;

    playersActed++;

    if (action == "FOLD") {
        AudioManager::instance().playSound(SoundEffect::Fold);
        p.hasFolded = true;
    } else if (action == "CALL") {
        AudioManager::instance().playSound(SoundEffect::ChipBet);
        int toCall = currentHighestBet - p.currentBet;
        placeBet(pIdx, std::min(toCall, p.balance));
    } else if (action == "RAISE") {
        AudioManager::instance().playSound(SoundEffect::ChipBet);
        int amountToAdd = raiseTotal - p.currentBet;
        if (amountToAdd > 0) placeBet(pIdx, std::min(amountToAdd, p.balance));
    } else if (action == "CHECK") {
        AudioManager::instance().playSound(SoundEffect::Check);
    }

    if (countActivePlayers() == 1) {
        for (auto& plr : players) {
            if (!plr.hasFolded) {
                plr.balance += pot;
                statusMessage = QString(getLocalizedText("%1 забирает банк (все сделали Fold).", "%1 takes the pot (all folded).")).arg(plr.name);
                gameOver = true;
                emit stateChanged();
                return;
            }
        }
    }

    currentTurnIdx = getNextActivePlayer(currentTurnIdx);
    checkPhaseAdvance();
}

void PokerEngine::checkPhaseAdvance() {
    bool allMatched = true;
    for (const auto& p : players) {
        if (!p.hasFolded && !p.isAllIn && p.currentBet < currentHighestBet) allMatched = false;
    }

    if ((allMatched && playersActed >= countActivePlayers()) || countNonAllInPlayers() <= 1) {
        for (const auto& p : players) {
            if (!p.hasFolded && !p.isAllIn && p.currentBet < currentHighestBet) return;
        }

        for (auto& p : players) p.currentBet = 0;
        currentHighestBet = 0;
        minRaise = bigBlind;
        playersActed = 0;

        if (phase == PREFLOP) {
            AudioManager::instance().playSound(SoundEffect::CardPlace);
            phase = FLOP;
            for (int i = 0; i < 3; ++i) communityCards.append(deck.takeFirst());
        } else if (phase == FLOP) {
            AudioManager::instance().playSound(SoundEffect::CardPlace);
            phase = TURN;
            communityCards.append(deck.takeFirst());
        } else if (phase == TURN) {
            AudioManager::instance().playSound(SoundEffect::CardPlace);
            phase = RIVER;
            communityCards.append(deck.takeFirst());
        } else {
            phase = SHOWDOWN;
            resolveShowdown();
            return;
        }

        if (countNonAllInPlayers() <= 1) {
            while (communityCards.size() < 5) communityCards.append(deck.takeFirst());
            phase = SHOWDOWN;
            resolveShowdown();
            return;
        }
        currentTurnIdx = getNextActivePlayer(dealerIdx);
    }
    updateStatus();
    emit stateChanged();
}

void PokerEngine::resolveShowdown() {
    AudioManager::instance().playSound(SoundEffect::Win);

    for (int i = 0; i < players.size(); ++i) {
        if (!players[i].hasFolded) {
            QVector<Card> totalCards = players[i].holeCards;
            totalCards.append(communityCards);
            players[i].bestHand = evaluate7Cards(totalCards);
        }
    }

    std::map<int, int> winnings;

    // Расчет Side Pots и основного банка
    while (pot > 0) {
        int minContr = 99999999;
        for (const auto& p : players) {
            if (p.totalContributed > 0 && p.totalContributed < minContr) {
                minContr = p.totalContributed;
            }
        }
        if (minContr == 99999999 || minContr <= 0) break;

        int segmentPot = 0;
        QVector<int> eligible;
        for (int i = 0; i < players.size(); ++i) {
            if (players[i].totalContributed > 0) {
                int take = std::min(players[i].totalContributed, minContr);
                segmentPot += take;
                players[i].totalContributed -= take;
                if (!players[i].hasFolded) {
                    eligible.append(i);
                }
            }
        }
        pot -= segmentPot;

        if (eligible.isEmpty()) break;

        unsigned int bestScore = 0;
        QVector<int> segWinners;
        for (int pIdx : eligible) {
            if (players[pIdx].bestHand.score > bestScore) {
                bestScore = players[pIdx].bestHand.score;
                segWinners.clear();
                segWinners.append(pIdx);
            } else if (players[pIdx].bestHand.score == bestScore) {
                segWinners.append(pIdx);
            }
        }

        int split = segmentPot / std::max(1, static_cast<int>(segWinners.size()));
        int rem = segmentPot % std::max(1, static_cast<int>(segWinners.size()));
        for (int w : segWinners) winnings[w] += split;
        if (rem > 0 && !segWinners.isEmpty()) winnings[segWinners.first()] += rem;
    }

    QStringList winSummaries;
    for (auto const& [pIdx, amt] : winnings) {
        players[pIdx].balance += amt;
        winSummaries.append(QString("%1 (+$%2)").arg(players[pIdx].name).arg(amt));
    }

    if (winnings.size() == 1) {
        int wIdx = winnings.begin()->first;
        statusMessage = QString(getLocalizedText("%1 победил!\n(%2)", "%1 won!\n(%2)")).arg(players[wIdx].name, players[wIdx].bestHand.name);
    } else {
        statusMessage = QString(getLocalizedText("Раздел банка:\n%1", "Split pot:\n%1")).arg(winSummaries.join(", "));
    }

    pot = 0;
    gameOver = true;
    emit stateChanged();
}

void PokerEngine::updateStatus() {
    if (gameOver) return;
    int toCall = currentHighestBet - players[currentTurnIdx].currentBet;
    if (toCall > 0) {
        statusMessage = QString(getLocalizedText("Ход: %1 | Колл: $%2", "Turn: %1 | Call: $%2")).arg(players[currentTurnIdx].name).arg(toCall);
    } else {
        statusMessage = QString(getLocalizedText("Ход: %1 | Чек", "Turn: %1 | Check")).arg(players[currentTurnIdx].name);
    }
    emit stateChanged();
}

bool PokerEngine::makeAiMove() {
    if (isProcessingMove || gameOver || currentTurnIdx >= players.size() || !players[currentTurnIdx].isBot) return false;

    isProcessingMove = true;
    int pIdx = currentTurnIdx;
    Player& bot = players[pIdx];
    if (bot.hasFolded || bot.isAllIn || bot.isBankrupt) {
        isProcessingMove = false;
        return false;
    }

    int toCall = currentHighestBet - bot.currentBet;
    int randVal = QRandomGenerator::global()->bounded(100);

    if (communityCards.isEmpty()) {
        if (bot.holeCards.size() < 2) {
            isProcessingMove = false;
            return false;
        }
        int r1 = bot.holeCards[0].rank;
        int r2 = bot.holeCards[1].rank;
        bool isPair = (r1 == r2);
        int highRank = std::max(r1, r2);

        if (toCall == 0) {
            if ((isPair || highRank >= 12) && randVal < 40) processAction(pIdx, "RAISE", currentHighestBet + minRaise);
            else processAction(pIdx, "CALL");
        } else {
            if (isPair && highRank >= 10) {
                if (randVal < 50) processAction(pIdx, "RAISE", currentHighestBet + minRaise);
                else processAction(pIdx, "CALL");
            } else if (isPair || highRank >= 11 || (r1 >= 9 && r2 >= 9)) {
                processAction(pIdx, "CALL");
            } else {
                if (toCall > bigBlind && randVal < 70) processAction(pIdx, "FOLD");
                else processAction(pIdx, "CALL");
            }
        }
        isProcessingMove = false;
        return true;
    }

    QVector<Card> hand = bot.holeCards;
    hand.append(communityCards);
    HandValue hv = evaluate7Cards(hand);

    if (toCall == 0) {
        if (hv.rank >= Pair && randVal < 40) processAction(pIdx, "RAISE", currentHighestBet + minRaise);
        else processAction(pIdx, "CALL");
    } else {
        if (hv.rank >= TwoPair) {
            if (randVal < 70) processAction(pIdx, "RAISE", currentHighestBet + minRaise);
            else processAction(pIdx, "CALL");
        } else if (hv.rank == Pair) {
            if (toCall > bot.balance / 2 && randVal < 60) processAction(pIdx, "FOLD");
            else processAction(pIdx, "CALL");
        } else {
            if (toCall > bigBlind && randVal < 80) processAction(pIdx, "FOLD");
            else processAction(pIdx, "CALL");
        }
    }

    isProcessingMove = false;
    return true;
}

QJsonObject PokerEngine::toJson(int targetId) const {
    QJsonObject json;
    json["yourId"] = targetId;
    json["pot"] = pot;
    json["phase"] = static_cast<int>(phase);
    json["currentHighestBet"] = currentHighestBet;
    json["currentTurnIdx"] = currentTurnIdx;
    json["dealerIdx"] = dealerIdx;
    json["minRaise"] = minRaise;
    json["gameOver"] = gameOver;
    json["statusMessage"] = statusMessage;

    QJsonArray commArr;
    for (const auto& c : communityCards) commArr.append(c.toJson());
    json["community"] = commArr;

    QJsonArray playersArr;
    for (const auto& p : players) {
        QJsonObject pObj;
        pObj["name"] = p.name;
        pObj["avatar"] = p.avatar;
        pObj["balance"] = p.balance;
        pObj["currentBet"] = p.currentBet;
        pObj["hasFolded"] = p.hasFolded;
        pObj["isAllIn"] = p.isAllIn;
        pObj["isBankrupt"] = p.isBankrupt;
        pObj["isDisconnected"] = p.isDisconnected;
        pObj["cardCount"] = p.holeCards.size();

        QJsonArray handArr;
        if (phase == SHOWDOWN || gameOver || p.id == targetId) {
            for (const auto& c : p.holeCards) handArr.append(c.toJson());
        }
        pObj["holeCards"] = handArr;
        playersArr.append(pObj);
    }
    json["players"] = playersArr;
    return json;
}

void PokerEngine::fromJson(const QJsonObject& json) {
    if (json.contains("yourId")) {
        myIdx = json["yourId"].toInt();
    }
    pot = json["pot"].toInt();
    phase = static_cast<Phase>(json["phase"].toInt());
    currentHighestBet = json["currentHighestBet"].toInt();
    currentTurnIdx = json["currentTurnIdx"].toInt();
    dealerIdx = json["dealerIdx"].toInt();
    minRaise = json["minRaise"].toInt();
    gameOver = json["gameOver"].toBool();
    statusMessage = json["statusMessage"].toString();

    communityCards.clear();
    for (auto val : json["community"].toArray()) {
        communityCards.append(Card::fromJson(val.toObject()));
    }

    QJsonArray pArr = json["players"].toArray();
    players.resize(pArr.size());
    for (int i = 0; i < pArr.size(); ++i) {
        QJsonObject pObj = pArr[i].toObject();
        players[i].name = pObj["name"].toString();
        players[i].avatar = pObj["avatar"].toInt(0);
        players[i].balance = pObj["balance"].toInt();
        players[i].currentBet = pObj["currentBet"].toInt();
        players[i].hasFolded = pObj["hasFolded"].toBool();
        players[i].isAllIn = pObj["isAllIn"].toBool();
        players[i].isBankrupt = pObj["isBankrupt"].toBool();
        players[i].isDisconnected = pObj["isDisconnected"].toBool();
        players[i].id = i;

        int cardCount = pObj["cardCount"].toInt(0);
        players[i].holeCards.clear();
        QJsonArray handArr = pObj["holeCards"].toArray();

        if (!handArr.isEmpty()) {
            for (auto val : handArr) {
                players[i].holeCards.append(Card::fromJson(val.toObject()));
            }
        } else {
            players[i].holeCards.resize(cardCount);
        }
    }
    emit stateChanged();
}
