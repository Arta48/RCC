#include "KozelWidget.h"
#include "Audio.h"

#include <QStackedWidget>
#include <QSet>

KozelWidget::KozelWidget(NetworkManager* netMgr, QWidget* parent)
: BaseTableWidget(parent), netManager(netMgr)
{
    btnPlayCards = new QPushButton(getLocalizedText("СДЕЛАТЬ ХОД", "PLAY CARDS"), this);
    btnPlayCards->setStyleSheet("QPushButton { background: #10B981; color: white; font-weight: bold; border-radius: 6px; padding: 6px; }");
    btnPlayCards->setCursor(Qt::PointingHandCursor);
    btnPlayCards->hide();

    connect(btnPlayCards, &QPushButton::clicked, this, [this]() {
        if (!selectedHandCardIndices.isEmpty()) {
            if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                QJsonObject json; json["act"] = "PLAY";
                QJsonArray arr;
                for (int idx : selectedHandCardIndices) arr.append(idx);
                json["cardIndices"] = arr;
                netManager->sendJsonToServer(json);
            } else {
                engine.playCards(engine.myIdx, selectedHandCardIndices);
                if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
            }
            selectedHandCardIndices.clear();
            updateUI();
        }
    });

    connect(btnNextHand, &QPushButton::clicked, this, [this]() {
        if (netManager && netManager->isNetworkGame && netManager->isHost) {
            const int activeClients = netManager->getActiveClientCount();
            if (activeClients == 0) {
                if (onReturnToLobbyCallback) onReturnToLobbyCallback();
                return;
            }

            engine.initGame(activeClients, true);
            for (int i = 0; i < netManager->lobbyClients.size() && i < engine.players.size(); ++i) {
                engine.players[i].name = netManager->lobbyClients[i].name;
                engine.players[i].avatar = netManager->lobbyClients[i].avatar;
            }
            broadcastNetState();
            updateUI();
            return;
        }

        engine.initGame(engine.players.size() - 1, false);
        updateUI();
    });

    aiTimer = new QTimer(this);
    connect(aiTimer, &QTimer::timeout, this, &KozelWidget::handleAiLogic);
    connect(&engine, &KozelEngine::stateChanged, this, &KozelWidget::updateUI);
}

void KozelWidget::startSingleGame(int botCount) {
    if (netManager) {
        netManager->disconnectAll();
    }
    engine.initGame(botCount);
    updateUI();
    aiTimer->start(800);
}

void KozelWidget::handleAiLogic() {
    if (parentWidget() && static_cast<QStackedWidget*>(parentWidget())->currentIndex() != 3) return;
    if (engine.isProcessingMove) return;

    if (!engine.gameOver && (!netManager || !netManager->isNetworkGame || netManager->isHost)) {
        if (engine.makeAiMove()) {
            if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
            updateUI();
        }
    }
}

void KozelWidget::updateUI() {
    if (engine.gameOver) {
        lblStatus->setText(engine.statusMessage);
    } else if (engine.myIdx >= 0 && engine.myIdx < engine.players.size() && engine.players[engine.myIdx].isOut) {
        lblStatus->setText(getLocalizedText("Вы зашли во время игры. Ожидание следующего раунда...", "You joined mid-game. Waiting for next round..."));
    } else if (engine.statusMessage.contains(getLocalizedText("забирает взятку", "takes trick"))) {
        const QString winnerName = engine.statusMessage.section(getLocalizedText(" забирает взятку", " takes trick"), 0, 0);
        if (engine.myIdx >= 0 && engine.myIdx < engine.players.size() && winnerName == engine.players[engine.myIdx].name) {
            const int pts = engine.statusMessage.section('+', 1).section(' ', 0, 0).toInt();
            lblStatus->setText(QString(getLocalizedText("Вы забираете взятку (+%1 очков) и ходите!", "You take the trick (+%1 pts) and lead!")).arg(pts));
        } else {
            lblStatus->setText(engine.statusMessage);
        }
    } else {
        if (engine.currentTurnIdx == engine.myIdx) {
            lblStatus->setText(getLocalizedText("Ваш ход! Заходите любой картой.", "Your turn! Lead with any card."));
        } else if (engine.currentTurnIdx >= 0 && engine.currentTurnIdx < engine.players.size()) {
            lblStatus->setText(QString(getLocalizedText("Ход игрока %1", "%1's turn")).arg(engine.players[engine.currentTurnIdx].name));
        } else {
            lblStatus->setText(engine.statusMessage);
        }
    }

    if (engine.currentTurnIdx != engine.myIdx) {
        selectedHandCardIndices.clear();
    }

    const int activeClients = netManager ? netManager->getActiveClientCount() : 0;
    const bool isHostOrSolo = (!netManager || !netManager->isNetworkGame || netManager->isHost);
    const bool isError = engine.statusMessage.contains(getLocalizedText("Ошибка", "Error")) || engine.statusMessage.contains(getLocalizedText("потеряна", "lost"));

    btnPlayCards->setVisible(engine.currentTurnIdx == engine.myIdx && !selectedHandCardIndices.isEmpty() && !engine.gameOver);
    btnNextHand->setVisible(engine.gameOver && isHostOrSolo && !isError); // Кнопка "Играть заново" скрыта при ошибке разрыва связи

    if (netManager && netManager->isNetworkGame && netManager->isHost) {
        if (activeClients == 0) {
            btnNextHand->setText(getLocalizedText("ВЕРНУТЬСЯ В ЛОББИ", "RETURN TO LOBBY"));
        } else {
            btnNextHand->setText(getLocalizedText("ИГРАТЬ ЗАНОВО", "PLAY AGAIN"));
        }
    } else {
        btnNextHand->setText(getLocalizedText("ИГРАТЬ ЗАНОВО", "PLAY AGAIN"));
    }

    update();
}

void KozelWidget::processNetAction(int senderId, const QJsonObject& json) {
    if (!netManager || !netManager->isHost) return;
    if (senderId < 0 || senderId >= NetConfig::MAX_PLAYERS) return;

    if (senderId >= engine.players.size()) {
        while (engine.players.size() <= senderId) {
            KozelPlayer p;
            p.id = engine.players.size();
            p.name = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].name : QString(getLocalizedText("Игрок %1", "Player %1")).arg(p.id + 1);
            p.avatar = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].avatar : 0;
            p.isBot = false;
            engine.players.append(p);
        }
    }

    const QString act = json["act"].toString();
    if (act == "PLAY") {
        if (senderId != engine.currentTurnIdx || engine.gameOver) return;

        const QJsonArray arr = json["cardIndices"].toArray();
        const auto& hand = engine.players[senderId].hand;

        QVector<int> indices;
        QSet<int> uniqueCheck;

        // Строгая валидация уникальности и границ индексов карт
        for (const auto& v : arr) {
            const int idx = v.toInt(-1);
            if (idx >= 0 && idx < hand.size() && !uniqueCheck.contains(idx)) {
                uniqueCheck.insert(idx);
                indices.append(idx);
            }
        }

        if (!indices.isEmpty()) {
            engine.playCards(senderId, indices);
        }
    }
    broadcastNetState();
}

void KozelWidget::broadcastNetState() {
    if (!netManager || !netManager->isHost) return;
    for (int i = 0; i < netManager->clientSockets.size(); ++i) {
        auto* socket = netManager->clientSockets[i];
        const int targetPlayerId = i + 1;
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            QJsonObject json = engine.toJson(targetPlayerId);
            json["isLobby"]  = false;
            json["gameType"] = 2;
            netManager->sendJsonToClient(i, json);
        }
    }
    updateUI();
}

void KozelWidget::resizeEvent(QResizeEvent* ev) {
    BaseTableWidget::resizeEvent(ev);
    const qreal s = getScale();

    const int btnW = qRound(150 * s);
    const int btnH = qRound(45 * s);
    btnPlayCards->setGeometry(width() - btnW - qRound(20 * s), height() - btnH - qRound(18 * s), btnW, btnH);
    btnPlayCards->setFont(QFont(font().family(), qMax(8, qRound(13 * s)), QFont::Bold));
}

void KozelWidget::mouseMoveEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty() || engine.myIdx >= engine.players.size()) return;
    const auto& myHand = engine.players[engine.myIdx].hand;
    const qreal s = getScale();

    const int cardW = qRound(80 * s);
    const int cardH = qRound(115 * s);
    const int handY = height() - cardH - qRound(110 * s);
    const int stepX = qMin(qRound(60 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
    const int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    int newHovered = -1;
    for (int i = myHand.size() - 1; i >= 0; --i) {
        const int offsetY = selectedHandCardIndices.contains(i) ? qRound(-25 * s) : ((i == hoveredHandCardIdx) ? qRound(-12 * s) : 0);
        if (QRect(startX + i * stepX, handY + offsetY, cardW, cardH).contains(ev->pos())) {
            newHovered = i;
            break;
        }
    }

    if (newHovered != hoveredHandCardIdx) {
        hoveredHandCardIdx = newHovered;
        update();
    }
}

void KozelWidget::mousePressEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty() || engine.currentTurnIdx != engine.myIdx) return;
    const auto& myHand = engine.players[engine.myIdx].hand;
    const qreal s = getScale();

    const int cardW = qRound(80 * s);
    const int cardH = qRound(115 * s);
    const int handY = height() - cardH - qRound(110 * s);
    const int stepX = qMin(qRound(60 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
    const int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    for (int i = myHand.size() - 1; i >= 0; --i) {
        const int offsetY = selectedHandCardIndices.contains(i) ? qRound(-25 * s) : ((i == hoveredHandCardIdx) ? qRound(-12 * s) : 0);
        if (QRect(startX + i * stepX, handY + offsetY, cardW, cardH).contains(ev->pos())) {
            if (selectedHandCardIndices.contains(i)) {
                if (!selectedHandCardIndices.isEmpty()) {
                    if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                        QJsonObject json; json["act"] = "PLAY";
                        QJsonArray arr;
                        for (int idx : selectedHandCardIndices) arr.append(idx);
                        json["cardIndices"] = arr;
                        netManager->sendJsonToServer(json);
                    } else {
                        engine.playCards(engine.myIdx, selectedHandCardIndices);
                        if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
                    }
                    selectedHandCardIndices.clear();
                }
            } else {
                if (!selectedHandCardIndices.isEmpty()) {
                    const Suit firstSuit = myHand[selectedHandCardIndices.first()].suit;
                    if (myHand[i].suit != firstSuit) selectedHandCardIndices.clear();
                }
                selectedHandCardIndices.append(i);
            }
            updateUI();
            return;
        }
    }
}

void KozelWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawTableFelt(p);

    const qreal s = getScale();

    if (!engine.players.isEmpty()) {
        const int cardW = qRound(80 * s);
        const int cardH = qRound(115 * s);

        p.setPen(QColor(255, 215, 0));
        p.setFont(QFont(font().family(), qMax(9, qRound(14 * s)), QFont::Bold));
        static const QString suitsStr[] = { "♥", "♦", "♣", "♠" };
        p.drawText(qRound(35 * s), qRound(80 * s), QString(getLocalizedText("Козырь: %1", "Trump: %1")).arg(suitsStr[engine.trumpSuit]));

        drawPlayers(p, cardW, cardH);

        const int trickStep = qRound(45 * s);
        const int trickStartX = width() / 2 - (engine.currentTrick.size() * trickStep) / 2;
        for (int t = 0; t < engine.currentTrick.size(); ++t) {
            drawCard(p, QRect(trickStartX + t * trickStep, height() / 2 - cardH / 2, cardW, cardH), &engine.currentTrick[t].second, true);
        }

        if (engine.myIdx < engine.players.size()) {
            const auto& myHand = engine.players[engine.myIdx].hand;
            const int handY = height() - cardH - qRound(110 * s);
            const int stepHandX = qMin(qRound(60 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
            const int startX = (width() - (myHand.size() * stepHandX + (cardW - stepHandX))) / 2;
            for (int i = 0; i < myHand.size(); ++i) {
                const bool isSelected = selectedHandCardIndices.contains(i);
                const bool isHovered  = (i == hoveredHandCardIdx);
                const int offsetY     = isSelected ? qRound(-25 * s) : (isHovered ? qRound(-12 * s) : 0);
                drawCard(p, QRect(startX + i * stepHandX, handY + offsetY, cardW, cardH), &myHand[i], true, isSelected);
            }
        }
    }

    if (engine.gameOver) {
        drawGameOverBanner(p, engine.statusMessage);
    }
}

void KozelWidget::drawPlayers(QPainter& p, int cardW, int cardH) {
    const int numPlayers = engine.players.size();
    if (numPlayers <= 0) return; // Защита от деления на 0

    const qreal s = getScale();
    const QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), qRound(75 * s), qRound(80 * s));

    const int boxW = qRound(150 * s);
    const int boxH = qRound(45 * s);

    for (int i = 0; i < numPlayers; ++i) {
        const int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        const QPoint pos = seatPos[displayIdx];
        const auto& plr = engine.players[i];

        if (engine.currentTurnIdx == i && !engine.gameOver) {
            p.setPen(QPen(QColor(59, 130, 246, 220), qMax(2, qRound(3 * s))));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(pos.x() - boxW / 2 - 4, pos.y() - boxH / 2 - 4, boxW + 8, boxH + 8, 8, 8);
        }

        p.setBrush(QColor(15, 25, 35, 230));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.drawRoundedRect(pos.x() - boxW / 2, pos.y() - boxH / 2, boxW, boxH, 6, 6);

        p.setPen(Qt::white);
        p.setFont(QFont(font().family(), qMax(8, qRound(11 * s)), QFont::Bold));
        const QString nameWithAvatar = getAvatarEmojiById(plr.avatar) + " " + plr.name;
        p.drawText(QRect(pos.x() - boxW / 2 + 6, pos.y() - boxH / 2 + 3, boxW - 12, boxH / 2), Qt::AlignLeft, nameWithAvatar);

        p.setPen(QColor(167, 243, 208));
        p.drawText(QRect(pos.x() - boxW / 2 + 6, pos.y(), boxW - 12, boxH / 2), Qt::AlignLeft, QString(getLocalizedText("Очки: %1", "Points: %1")).arg(plr.pointsCollected));

        if (displayIdx != 0) {
            const int handSize = plr.hand.size();
            const int oppStep = qRound(15 * s);
            const int oppW = cardW - qRound(25 * s);
            const int oppH = cardH - qRound(35 * s);
            const int startX = pos.x() - (handSize * oppStep + (oppW - oppStep)) / 2;
            for (int c = 0; c < handSize; ++c) {
                drawCard(p, QRect(startX + c * oppStep, pos.y() + boxH / 2 + qRound(5 * s), oppW, oppH), nullptr, false);
            }
        }
    }
}
