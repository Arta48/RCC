#include "DurakWidget.h"
#include "Audio.h"

#include <QStackedWidget>

DurakWidget::DurakWidget(NetworkManager* netMgr, QWidget* parent)
: BaseTableWidget(parent), netManager(netMgr)
{
    btnPass = new QPushButton(getLocalizedText("БИТО / ПАС", "DONE / PASS"), this);
    btnTake = new QPushButton(getLocalizedText("ВЗЯТЬ КАРТЫ", "TAKE CARDS"), this);

    btnPass->setStyleSheet("QPushButton { background: #3B82F6; color: white; font-weight: bold; border-radius: 6px; padding: 10px; }");
    btnTake->setStyleSheet("QPushButton { background: #F59E0B; color: white; font-weight: bold; border-radius: 6px; padding: 10px; }");

    btnPass->setCursor(Qt::PointingHandCursor);
    btnTake->setCursor(Qt::PointingHandCursor);

    connect(btnPass, &QPushButton::clicked, this, [this]() {
        if (netManager && netManager->isNetworkGame && !netManager->isHost) {
            QJsonObject json; json["act"] = "PASS";
            netManager->sendJsonToServer(json);
        } else {
            engine.passAction();
            if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
        }
        updateUI();
    });

    connect(btnTake, &QPushButton::clicked, this, [this]() {
        if (netManager && netManager->isNetworkGame && !netManager->isHost) {
            QJsonObject json; json["act"] = "TAKE";
            netManager->sendJsonToServer(json);
        } else {
            engine.takeAction();
            if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
        }
        updateUI();
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
    connect(aiTimer, &QTimer::timeout, this, &DurakWidget::handleAiLogic);
    connect(&engine, &DurakEngine::stateChanged, this, &DurakWidget::updateUI);
}

void DurakWidget::startSingleGame(int botCount) {
    if (netManager) {
        netManager->disconnectAll();
    }
    engine.initGame(botCount);
    updateUI();
    aiTimer->start(700);
}

void DurakWidget::handleAiLogic() {
    if (parentWidget() && static_cast<QStackedWidget*>(parentWidget())->currentIndex() != 2) return;
    if (engine.isProcessingMove) return;

    if (!engine.gameOver && (!netManager || !netManager->isNetworkGame || netManager->isHost)) {
        if (engine.makeAiMove()) {
            if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
            updateUI();
        }
    }
}

void DurakWidget::updateUI() {
    bool canPressBito = false;
    bool canPressTake = false;
    bool allDefended = true;
    int undefendedCount = 0;

    // Безопасная проверка: стол и список игроков не должны быть пустыми
    if (!engine.table.isEmpty() && !engine.players.isEmpty()) {
        for (const auto& pair : engine.table) {
            if (!pair.isDefended) {
                allDefended = false;
                undefendedCount++;
            }
        }

        // Безопасное получение карт защитника
        int defenderCardsCount = 0;
        if (engine.defenderIdx >= 0 && engine.defenderIdx < engine.players.size()) {
            defenderCardsCount = engine.players[engine.defenderIdx].hand.size();
        }
        const bool limitReached = (undefendedCount >= defenderCardsCount || engine.table.size() >= 6);
        const bool hasMyCards = (engine.myIdx >= 0 && engine.myIdx < engine.players.size() && !engine.players[engine.myIdx].hand.isEmpty());

        if (engine.isDefenderTaking) {
            if (!limitReached && hasMyCards &&
                (engine.attackerIdx == engine.myIdx || engine.currentTurnIdx == engine.myIdx)) {
                canPressBito = true;
                }
        } else {
            if (allDefended && engine.attackerIdx == engine.myIdx && hasMyCards) {
                canPressBito = true;
            }
            if (!allDefended && engine.defenderIdx == engine.myIdx) {
                canPressTake = true;
            }
        }
    }

    const int activeClients = netManager ? netManager->getActiveClientCount() : 0;
    const bool isHostOrSolo = (!netManager || !netManager->isNetworkGame || netManager->isHost);
    const bool isError = engine.statusMessage.contains(getLocalizedText("Ошибка", "Error")) || engine.statusMessage.contains(getLocalizedText("потеряна", "lost"));

    btnPass->setVisible(canPressBito && !engine.gameOver);
    btnTake->setVisible(canPressTake && !engine.gameOver);
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

    if (engine.gameOver) {
        lblStatus->setText(engine.statusMessage);
    } else if (engine.myIdx >= 0 && engine.myIdx < engine.players.size() && engine.players[engine.myIdx].isOut && !engine.deck.isEmpty()) {
        lblStatus->setText(getLocalizedText("Вы зашли во время игры. Ожидание следующего раунда...", "You joined mid-game. Waiting for next round..."));
    } else if (engine.isDefenderTaking) {
        if (engine.defenderIdx == engine.myIdx) {
            lblStatus->setText(getLocalizedText("Вы берёте карты! Ожидание завершения хода...", "You take cards! Waiting for turn to finish..."));
        } else if (engine.myIdx >= 0 && engine.myIdx < engine.players.size() && engine.players[engine.myIdx].hand.isEmpty()) {
            const QString defName = (engine.defenderIdx >= 0 && engine.defenderIdx < engine.players.size()) ? engine.players[engine.defenderIdx].name : "";
            lblStatus->setText(QString(getLocalizedText("%1 берёт карты!", "%1 takes cards!")).arg(defName));
        } else if (engine.attackerIdx == engine.myIdx || engine.currentTurnIdx == engine.myIdx) {
            const QString defName = (engine.defenderIdx >= 0 && engine.defenderIdx < engine.players.size()) ? engine.players[engine.defenderIdx].name : "";
            lblStatus->setText(QString(getLocalizedText("%1 берёт карты! Подкиньте или нажмите Пас", "%1 takes cards! Toss cards or press Pass")).arg(defName));
        } else {
            const QString defName = (engine.defenderIdx >= 0 && engine.defenderIdx < engine.players.size()) ? engine.players[engine.defenderIdx].name : "";
            lblStatus->setText(QString(getLocalizedText("%1 берёт карты!", "%1 takes cards!")).arg(defName));
        }
    } else if (engine.table.isEmpty()) {
        if (engine.attackerIdx == engine.myIdx) {
            lblStatus->setText(getLocalizedText("Ваш ход! Атакуйте!", "Your turn! Attack!"));
        } else if (engine.defenderIdx == engine.myIdx) {
            const QString atkName = (engine.attackerIdx >= 0 && engine.attackerIdx < engine.players.size()) ? engine.players[engine.attackerIdx].name : "";
            lblStatus->setText(QString(getLocalizedText("Ожидание атаки от игрока %1...", "Waiting for %1 to attack...")).arg(atkName));
        } else {
            const QString atkName = (engine.attackerIdx >= 0 && engine.attackerIdx < engine.players.size()) ? engine.players[engine.attackerIdx].name : "";
            lblStatus->setText(QString(getLocalizedText("Ход игрока %1", "%1's turn")).arg(atkName));
        }
    } else {
        if (!allDefended) {
            if (engine.defenderIdx == engine.myIdx) {
                lblStatus->setText(getLocalizedText("Ваш ход! Защищайтесь!", "Your turn! Defend!"));
            } else if (engine.attackerIdx == engine.myIdx) {
                const QString defName = (engine.defenderIdx >= 0 && engine.defenderIdx < engine.players.size()) ? engine.players[engine.defenderIdx].name : "";
                lblStatus->setText(QString(getLocalizedText("Ожидание защиты от игрока %1...", "Waiting for %1 to defend...")).arg(defName));
            } else {
                const QString defName = (engine.defenderIdx >= 0 && engine.defenderIdx < engine.players.size()) ? engine.players[engine.defenderIdx].name : "";
                lblStatus->setText(QString(getLocalizedText("%1 защищается...", "%1 is defending...")).arg(defName));
            }
        } else {
            if (engine.attackerIdx == engine.myIdx) {
                lblStatus->setText(getLocalizedText("Все карты отбиты! Подкиньте или нажмите Бито", "All cards beaten! Toss cards or press Done"));
            } else if (engine.defenderIdx == engine.myIdx) {
                const QString atkName = (engine.attackerIdx >= 0 && engine.attackerIdx < engine.players.size()) ? engine.players[engine.attackerIdx].name : "";
                lblStatus->setText(QString(getLocalizedText("Ожидание хода %1 (подкинет или Бито)...", "Waiting for %1 (toss or Done)...")).arg(atkName));
            } else {
                const QString atkName = (engine.attackerIdx >= 0 && engine.attackerIdx < engine.players.size()) ? engine.players[engine.attackerIdx].name : "";
                lblStatus->setText(QString(getLocalizedText("%1 подкидывает или Бито...", "%1 is tossing or Done...")).arg(atkName));
            }
        }
    }

    update();
}

void DurakWidget::processNetAction(int senderId, const QJsonObject& json) {
    if (!netManager || !netManager->isHost) return;
    if (senderId < 0 || senderId >= NetConfig::MAX_PLAYERS) return;

    if (senderId >= engine.players.size()) {
        while (engine.players.size() <= senderId) {
            DurakPlayer p;
            p.id = engine.players.size();
            p.name = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].name : QString(getLocalizedText("Игрок %1", "Player %1")).arg(p.id + 1);
            p.avatar = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].avatar : 0;
            p.isBot = false;
            p.isOut = true;
            engine.players.append(p);
        }
    }

    const QString act = json["act"].toString();
    const auto& hand = engine.players[senderId].hand;

    // Строгая валидация сетевых пакетов для Подкидного Дурака
    if (act == "ATTACK") {
        const int cardHandIdx = json["cardHandIdx"].toInt(-1);
        if (cardHandIdx >= 0 && cardHandIdx < hand.size()) {
            engine.playAttackCard(senderId, cardHandIdx);
        }
    } else if (act == "DEFEND") {
        const int cardHandIdx = json["cardHandIdx"].toInt(-1);
        const int tableIdx = json["tableIdx"].toInt(-1);
        if (senderId == engine.defenderIdx && cardHandIdx >= 0 && cardHandIdx < hand.size() &&
            tableIdx >= 0 && tableIdx < engine.table.size()) {
            engine.playDefendCard(senderId, cardHandIdx, tableIdx);
            }
    } else if (act == "PASS") {
        engine.passAction();
    } else if (act == "TAKE") {
        if (senderId == engine.defenderIdx) {
            engine.takeAction();
        }
    }
    broadcastNetState();
}

void DurakWidget::broadcastNetState() {
    if (!netManager || !netManager->isHost) return;
    for (int i = 0; i < netManager->clientSockets.size(); ++i) {
        auto* socket = netManager->clientSockets[i];
        const int targetPlayerId = i + 1;
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            QJsonObject json = engine.toJson(targetPlayerId);
            json["isLobby"]  = false;
            json["gameType"] = 1;
            netManager->sendJsonToClient(i, json);
        }
    }
    updateUI();
}

void DurakWidget::resizeEvent(QResizeEvent* ev) {
    BaseTableWidget::resizeEvent(ev);
    const qreal s = getScale();

    const int btnW = qRound(120 * s);
    const int btnH = qRound(45 * s);
    const int btnY = height() - btnH - qRound(18 * s);

    btnPass->setGeometry(width() - btnW * 2 - qRound(25 * s), btnY, btnW, btnH);
    btnTake->setGeometry(width() - btnW - qRound(15 * s), btnY, btnW, btnH);

    const QFont btnFont(font().family(), qMax(8, qRound(12 * s)), QFont::Bold);
    btnPass->setFont(btnFont);
    btnTake->setFont(btnFont);
}

void DurakWidget::mouseMoveEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty() || engine.myIdx >= engine.players.size()) return;
    const auto& myHand = engine.players[engine.myIdx].hand;
    const qreal s = getScale();

    const int cardW = qRound(80 * s);
    const int cardH = qRound(115 * s);
    const int handY = height() - cardH - qRound(110 * s);
    const int stepX = qMin(qRound(50 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
    const int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    int newHovered = -1;
    for (int i = myHand.size() - 1; i >= 0; --i) {
        const int offsetY = (i == selectedHandCardIdx) ? qRound(-25 * s) : ((i == hoveredHandCardIdx) ? qRound(-12 * s) : 0);
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

void DurakWidget::mousePressEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty() || engine.myIdx >= engine.players.size()) return;
    const auto& myHand = engine.players[engine.myIdx].hand;
    const qreal s = getScale();

    const int cardW = qRound(80 * s);
    const int cardH = qRound(115 * s);
    const int handY = height() - cardH - qRound(110 * s);
    const int stepX = qMin(qRound(50 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
    const int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    for (int i = myHand.size() - 1; i >= 0; --i) {
        const int offsetY = (i == selectedHandCardIdx) ? qRound(-25 * s) : ((i == hoveredHandCardIdx) ? qRound(-12 * s) : 0);
        if (QRect(startX + i * stepX, handY + offsetY, cardW, cardH).contains(ev->pos())) {
            const bool canAttack = (engine.attackerIdx == engine.myIdx) ||
            (engine.isDefenderTaking && engine.currentTurnIdx == engine.myIdx);

            if (canAttack) {
                selectedHandCardIdx = i;
                if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                    QJsonObject json; json["act"] = "ATTACK"; json["cardHandIdx"] = selectedHandCardIdx;
                    netManager->sendJsonToServer(json);
                    selectedHandCardIdx = -1;
                } else {
                    if (engine.playAttackCard(engine.myIdx, selectedHandCardIdx)) {
                        selectedHandCardIdx = -1;
                        if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
                    }
                }
                updateUI();
                return;
            }

            if (engine.defenderIdx == engine.myIdx && !engine.isDefenderTaking) {
                if (selectedHandCardIdx == i) {
                    selectedHandCardIdx = -1;
                } else {
                    selectedHandCardIdx = i;
                    QVector<int> undefended;
                    for (int t = 0; t < engine.table.size(); ++t) {
                        if (!engine.table[t].isDefended) undefended.append(t);
                    }

                    if (undefended.size() == 1) {
                        const int targetTableIdx = undefended.first();
                        if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                            QJsonObject json; json["act"] = "DEFEND"; json["cardHandIdx"] = selectedHandCardIdx; json["tableIdx"] = targetTableIdx;
                            netManager->sendJsonToServer(json);
                            selectedHandCardIdx = -1;
                        } else {
                            if (engine.playDefendCard(engine.myIdx, selectedHandCardIdx, targetTableIdx)) {
                                selectedHandCardIdx = -1;
                                if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
                            }
                        }
                    }
                }
                updateUI();
                return;
            }
            updateUI();
            return;
        }
    }

    if (selectedHandCardIdx != -1 && engine.defenderIdx == engine.myIdx && !engine.isDefenderTaking) {
        const int tableY = height() / 2 - cardH / 2;
        const int stepTableX = qRound(110 * s);
        const int totalTableW = engine.table.size() * stepTableX;
        const int tableStartX = (width() - totalTableW) / 2;
        for (int t = 0; t < engine.table.size(); ++t) {
            if (QRect(tableStartX + t * stepTableX, tableY, cardW, cardH).contains(ev->pos()) && !engine.table[t].isDefended) {
                if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                    QJsonObject json; json["act"] = "DEFEND"; json["cardHandIdx"] = selectedHandCardIdx; json["tableIdx"] = t;
                    netManager->sendJsonToServer(json);
                    selectedHandCardIdx = -1;
                } else {
                    if (engine.playDefendCard(engine.myIdx, selectedHandCardIdx, t)) {
                        selectedHandCardIdx = -1;
                        if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
                    }
                }
                updateUI();
                return;
            }
        }
    }
}

void DurakWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawTableFelt(p);

    const qreal s = getScale();
    const int leftOffset = getSafeLeftMargin();

    if (!engine.players.isEmpty()) {
        const int cardW = qRound(80 * s);
        const int cardH = qRound(115 * s);

        if (!engine.deck.isEmpty()) {
            drawCard(p, QRect(leftOffset, qRound(125 * s), cardH, cardW), &engine.trumpCard, true);
            drawCard(p, QRect(leftOffset + qRound(40 * s), qRound(95 * s), cardW, cardH), nullptr, false);

            p.setPen(Qt::white);
            p.setFont(QFont(font().family(), qMax(8, qRound(11 * s)), QFont::Bold));
            p.drawText(leftOffset + qRound(20 * s), qRound(225 * s), QString(getLocalizedText("Карт: %1", "Cards: %1")).arg(engine.deck.size()));
        } else {
            p.setPen(QColor(255, 235, 59));
            p.setFont(QFont(font().family(), qMax(9, qRound(13 * s)), QFont::Bold));
            static const QString suitsStr[] = { "♥", "♦", "♣", "♠" };
            p.drawText(leftOffset + qRound(20 * s), qRound(120 * s), QString(getLocalizedText("Козырь: %1", "Trump: %1")).arg(suitsStr[engine.trumpCard.suit]));
        }

        if (engine.bitoCount > 0) {
            p.save();
            p.translate(width() - qRound(80 * s), qRound(150 * s));
            p.rotate(15);
            drawCard(p, QRect(-cardW / 2, -cardH / 2, cardW, cardH), nullptr, false);
            p.restore();

            p.setPen(Qt::white);
            p.setFont(QFont(font().family(), qMax(8, qRound(11 * s)), QFont::Bold));
            p.drawText(width() - qRound(110 * s), qRound(225 * s), QString(getLocalizedText("Бито: %1", "Discards: %1")).arg(engine.bitoCount));
        }

        drawPlayers(p, cardW, cardH);

        const int tableY = height() / 2 - cardH / 2;
        const int stepX = qRound(110 * s);
        const int totalTableW = engine.table.size() * stepX;
        const int tableStartX = (width() - totalTableW) / 2;

        for (int t = 0; t < engine.table.size(); ++t) {
            const int cardX = tableStartX + t * stepX;
            drawCard(p, QRect(cardX, tableY, cardW, cardH), &engine.table[t].attack, true);
            if (engine.table[t].isDefended) {
                drawCard(p, QRect(cardX + qRound(22 * s), tableY + qRound(22 * s), cardW, cardH), &engine.table[t].defend, true);
            }
        }

        if (engine.myIdx < engine.players.size()) {
            const auto& myHand = engine.players[engine.myIdx].hand;
            const int handY = height() - cardH - qRound(110 * s);
            const int stepHandX = qMin(qRound(50 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
            const int startX = (width() - (myHand.size() * stepHandX + (cardW - stepHandX))) / 2;
            for (int i = 0; i < myHand.size(); ++i) {
                const bool isSelected = (i == selectedHandCardIdx);
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

void DurakWidget::drawPlayers(QPainter& p, int cardW, int cardH) {
    const int numPlayers = engine.players.size();
    if (numPlayers <= 0) return; // Защита от деления на 0

    const qreal s = getScale();
    const QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), qRound(75 * s), qRound(80 * s));

    const int boxW = qRound(160 * s);
    const int boxH = qRound(42 * s);

    for (int i = 0; i < numPlayers; ++i) {
        const int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        const QPoint pos = seatPos[displayIdx];
        const auto& opp = engine.players[i];

        if ((engine.attackerIdx == i || engine.defenderIdx == i) && !engine.gameOver) {
            p.setPen(QPen(QColor(59, 130, 246, 220), qMax(2, qRound(3 * s))));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(pos.x() - boxW / 2 - 4, pos.y() - boxH / 2 - 4, boxW + 8, boxH + 8, 8, 8);
        }

        p.setBrush(QColor(15, 25, 35, 230));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.drawRoundedRect(pos.x() - boxW / 2, pos.y() - boxH / 2, boxW, boxH, 6, 6);

        p.setPen(Qt::white);
        p.setFont(QFont(font().family(), qMax(8, qRound(11 * s)), QFont::Bold));
        const QString nameWithAvatar = getAvatarEmojiById(opp.avatar) + " " + opp.name;
        p.drawText(QRect(pos.x() - boxW / 2, pos.y() - boxH / 2, boxW, boxH), Qt::AlignCenter, nameWithAvatar);

        if (displayIdx != 0) {
            const int handSize = opp.hand.size();
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
