#include "PokerWidget.h"
#include "Audio.h"

#include <QStackedWidget>

PokerWidget::PokerWidget(NetworkManager* netMgr, QWidget* parent)
: BaseTableWidget(parent), netManager(netMgr)
{
    btnFold          = new QPushButton(getLocalizedText("FOLD", "FOLD"), this);
    btnCall          = new QPushButton(getLocalizedText("CALL", "CALL"), this);
    btnRaise         = new QPushButton(getLocalizedText("RAISE", "RAISE"), this);
    btnStartNetGame  = new QPushButton(getLocalizedText("НАЧАТЬ СЕТЕВУЮ ИГРУ", "START NETWORK GAME"), this);

    raiseSlider    = new QSlider(Qt::Horizontal, this);
    lblRaiseAmount = new QLabel("$0", this);

    const QString btnStyle = "QPushButton { background: %1; color: white; font-weight: bold; border-radius: 8px; border: none; } QPushButton:hover { background: %2; }";
    btnFold->setStyleSheet(btnStyle.arg("#EF4444", "#F87171"));
    btnCall->setStyleSheet(btnStyle.arg("#3B82F6", "#60A5FA"));
    btnRaise->setStyleSheet(btnStyle.arg("#F59E0B", "#FBBF24"));
    btnStartNetGame->setStyleSheet(btnStyle.arg("#8B5CF6", "#A78BFA"));

    lblRaiseAmount->setStyleSheet("QLabel { color: #FCD34D; font-weight: bold; background: rgba(0,0,0,0.5); border-radius: 4px; padding: 4px; }");
    lblRaiseAmount->setAlignment(Qt::AlignCenter);

    btnFold->setCursor(Qt::PointingHandCursor);
    btnCall->setCursor(Qt::PointingHandCursor);
    btnRaise->setCursor(Qt::PointingHandCursor);
    btnStartNetGame->setCursor(Qt::PointingHandCursor);

    connect(raiseSlider, &QSlider::valueChanged, this, [this](int val) {
        if (val < raiseSlider->maximum()) {
            const int snapped = (val / 10) * 10;
            if (snapped != val) {
                raiseSlider->setValue(snapped);
                return;
            }
        }
        lblRaiseAmount->setText(QString("$%1").arg(val));

        // Динамическая смена надписи: на максималке пишем ALL IN, в остальных случаях RAISE
        if (val >= raiseSlider->maximum()) {
            btnRaise->setText(getLocalizedText("ALL IN", "ALL IN"));
        } else {
            btnRaise->setText(getLocalizedText("RAISE", "RAISE"));
        }
    });

    connect(btnFold, &QPushButton::clicked, this, [this](){ onPlayerAction("FOLD"); });
    connect(btnCall, &QPushButton::clicked, this, [this](){ onPlayerAction("CALL"); });
    connect(btnRaise, &QPushButton::clicked, this, [this]() {
        if (engine.currentTurnIdx >= engine.players.size()) return;
        const Player& p = engine.players[engine.myIdx];
        const int minR = engine.currentHighestBet + engine.minRaise;
        const int maxR = p.balance + p.currentBet;

        if (!raiseSlider->isVisible() || minR >= maxR) {
            onPlayerAction("RAISE", maxR);
        } else {
            onPlayerAction("RAISE", raiseSlider->value());
        }
    });

    connect(btnStartNetGame, &QPushButton::clicked, this, [this]() {
        if (netManager && netManager->isHost && netManager->getActiveClientCount() >= 1) {
            const int gType = netManager->gameType;
            netManager->isLobby = false;
            if (gType == 0) {
                engine.initGame(netManager->getActiveClientCount(), true);

                for (int i = 0; i < netManager->lobbyClients.size() && i < engine.players.size(); ++i) {
                    engine.players[i].name   = netManager->lobbyClients[i].name;
                    engine.players[i].avatar = netManager->lobbyClients[i].avatar;
                }

                broadcastNetState();
                updateUI();
            } else {
                emit netManager->signalStartNetworkGame(gType, netManager->getActiveClientCount());
            }
        }
    });

    auto handleNextHandLambda = [this]() {
        lblStatus->setText(engine.statusMessage);
        if (netManager && netManager->isNetworkGame) {
            if (netManager->isHost) {
                const int activeClients = netManager->getActiveClientCount();
                if (activeClients == 0) {
                    netManager->isLobby = true;
                    engine.gameOver = false;
                    engine.players.resize(1);
                    engine.players[0].id = 0;
                    engine.players[0].name = AppSettings::instance().getNickname();
                    engine.players[0].avatar = static_cast<int>(AppSettings::instance().getAvatar());
                    engine.communityCards.clear();
                    engine.pot = 0;

                    // Очищаем сокеты и оставляем в лобби только хоста
                    netManager->clientSockets.clear();
                    if (!netManager->lobbyClients.isEmpty()) {
                        netManager->lobbyClients.resize(1);
                        netManager->lobbyClients[0].isDisconnected = false;
                    }

                    engine.statusMessage = QString(getLocalizedText("ЛОББИ: 1/%1 игроков. Ожидание...", "LOBBY: 1/%1 players. Waiting...")).arg(NetConfig::MAX_PLAYERS);
                    lblStatus->setText(engine.statusMessage);
                    broadcastNetState();
                    updateUI();
                    return;
                }

                if (engine.countSolventPlayers() < 2) {
                    engine.resetGame();
                } else {
                    engine.startNewHand();
                }

                broadcastNetState();
                updateUI();
            }
        } else {
            const bool isSoloHumanBankrupt = (engine.myIdx < engine.players.size() && engine.players[engine.myIdx].balance <= 0);
            if (isSoloHumanBankrupt || engine.countSolventPlayers() < 2) {
                engine.resetGame();
            } else {
                engine.startNewHand();
            }
            updateUI();
        }
    };

    connect(btnNextHand, &QPushButton::clicked, this, [this, handleNextHandLambda]() {
        autoNextHandTimer->stop();
        handleNextHandLambda();
    });

    aiTimer = new QTimer(this);
    connect(aiTimer, &QTimer::timeout, this, &PokerWidget::handleAiLogic);
    connect(&engine, &PokerEngine::stateChanged, this, &PokerWidget::updateUI);

    if (netManager) {
        connect(netManager, &NetworkManager::lobbyStatusChanged, this, [this](const QString& msg) {
            lblStatus->setText(msg);

            if (msg.contains(getLocalizedText("Ошибка", "Error")) || msg.contains(getLocalizedText("потеряна", "lost"))) {
                engine.gameOver = true;
                engine.statusMessage = msg;
            } else {
                // Сбрасываем флаг ошибки и баннер при успешном статусе лобби
                engine.gameOver = false;
                engine.statusMessage.clear();
            }

            updateUI();
        });
    }

    autoNextHandTimer = new QTimer(this);
    autoNextHandTimer->setSingleShot(true);
    connect(autoNextHandTimer, &QTimer::timeout, this, [this, handleNextHandLambda]() {
        if (!engine.gameOver) return;
        if (netManager && netManager->isNetworkGame) {
            if (netManager->isHost && !netManager->isLobby && engine.countSolventPlayers() >= 2) {
                handleNextHandLambda();
            }
        } else {
            const bool isSoloHumanBankrupt = (engine.myIdx < engine.players.size() && engine.players[engine.myIdx].balance <= 0);
            if (!isSoloHumanBankrupt && engine.countSolventPlayers() >= 2) {
                handleNextHandLambda();
            }
        }
    });
}

void PokerWidget::startSingleGame(int botCount) {
    if (netManager) {
        netManager->disconnectAll();
    }
    engine.initGame(botCount, false);
    updateUI();
    aiTimer->start(1500);
}

void PokerWidget::processNetAction(int senderId, const QJsonObject& json) {
    if (!netManager || !netManager->isHost) return;
    if (senderId < 0 || senderId >= NetConfig::MAX_PLAYERS) return;

    if (senderId >= engine.players.size()) {
        while (engine.players.size() <= senderId) {
            Player p;
            p.id = engine.players.size();
            p.name = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].name : QString(getLocalizedText("Игрок %1", "Player %1")).arg(p.id + 1);
            p.avatar = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].avatar : 0;
            p.isBot = false;
            p.hasFolded = true;
            p.balance = PokerConfig::DEFAULT_BALANCE;
            engine.players.append(p);
        }
    }

    const QString act = json["action"].toString();
    if (act == "JOIN") {
        broadcastNetState();
        return;
    }

    // Валидация очереди хода
    if (senderId != engine.currentTurnIdx || engine.gameOver) return;

    static const QStringList validActions = { "FOLD", "CALL", "RAISE", "CHECK" };
    if (!validActions.contains(act)) return;

    int amt = json["amount"].toInt(0);
    const Player& p = engine.players[senderId];

    if (act == "RAISE") {
        const int maxR = p.balance + p.currentBet;
        int minR = engine.currentHighestBet + engine.minRaise;
        // Если стек игрока меньше минимального рейза, олл-ин разрешен на весь стек
        if (minR > maxR) minR = maxR;
        amt = qBound(minR, amt, maxR);
    }

    engine.processAction(senderId, act, amt);
    broadcastNetState();
    updateUI(); // Мгновенно обновляем интерфейс хоста
}

void PokerWidget::broadcastNetState() {
    if (!netManager || !netManager->isHost) return;
    for (int i = 0; i < netManager->clientSockets.size(); ++i) {
        auto* socket = netManager->clientSockets[i];
        const int targetPlayerId = i + 1;
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            QJsonObject json = engine.toJson(targetPlayerId);
            json["isLobby"]  = false;
            json["gameType"] = 0;
            netManager->sendJsonToClient(i, json);
        }
    }
}

void PokerWidget::onPlayerAction(const QString& action, int raiseTotal) {
    if (engine.myIdx >= engine.players.size()) return;
    if (engine.currentTurnIdx >= engine.players.size()) return;
    if (engine.currentTurnIdx != engine.myIdx || engine.gameOver || (netManager && netManager->isLobby)) return;

    if (!netManager || !netManager->isNetworkGame || netManager->isHost) {
        engine.processAction(engine.myIdx, action, raiseTotal);
        if (netManager && netManager->isNetworkGame) broadcastNetState();
    } else {
        QJsonObject json;
        json["action"] = action;
        json["amount"] = raiseTotal;
        netManager->sendJsonToServer(json);
    }
    updateUI();
}

void PokerWidget::handleAiLogic() {
    if (parentWidget() && static_cast<QStackedWidget*>(parentWidget())->currentIndex() != 1) return;
    if (engine.isProcessingMove) return;

    if (!engine.gameOver && (!netManager || !netManager->isNetworkGame || netManager->isHost) && (!netManager || !netManager->isLobby)) {
        if (engine.currentTurnIdx < engine.players.size() && engine.players[engine.currentTurnIdx].isBot) {
            if (engine.makeAiMove()) {
                if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
                updateUI();
            }
        }
    }
}

void PokerWidget::updateUI() {
    const bool isLobby = (netManager && netManager->isNetworkGame && netManager->isLobby);
    const bool isMyTurn = (!engine.gameOver && !isLobby && engine.currentTurnIdx == engine.myIdx);

    btnFold->setVisible(isMyTurn);
    btnCall->setVisible(isMyTurn);
    btnRaise->setVisible(isMyTurn);
    raiseSlider->setVisible(isMyTurn);
    lblRaiseAmount->setVisible(isMyTurn);

    const int activeClients = netManager ? netManager->getActiveClientCount() : 0;
    const bool isHostOrSolo = (!netManager || !netManager->isNetworkGame || (netManager->isHost && !isLobby));
    const bool isError = engine.statusMessage.contains(getLocalizedText("Ошибка", "Error")) ||
    engine.statusMessage.contains(getLocalizedText("потеряна", "lost"));

    btnStartNetGame->setVisible(netManager && netManager->isHost && isLobby && activeClients >= 1);

    const bool canShowNextHand = engine.gameOver && isHostOrSolo && !isError;
    btnNextHand->setVisible(canShowNextHand);

    if (canShowNextHand) {
        const bool isNetworkHost = (netManager && netManager->isNetworkGame && netManager->isHost);
        const bool isSoloBankrupt = (!isNetworkHost && engine.myIdx < engine.players.size() && engine.players[engine.myIdx].balance <= 0);
        const bool needsFullReset = isSoloBankrupt || (engine.countSolventPlayers() < 2);

        if (isNetworkHost && activeClients == 0) {
            btnNextHand->setText(getLocalizedText("ВЕРНУТЬСЯ В ЛОББИ", "RETURN TO LOBBY"));
            autoNextHandTimer->stop();
        } else if (needsFullReset) {
            btnNextHand->setText(getLocalizedText("ИГРАТЬ ЗАНОВО", "PLAY AGAIN"));
            autoNextHandTimer->stop();
        } else {
            btnNextHand->setText(getLocalizedText("СЛЕДУЮЩАЯ РАЗДАЧА", "NEXT HAND"));
            if (!autoNextHandTimer->isActive() && AppSettings::instance().getAutoNextHand()) {
                autoNextHandTimer->start(3000);
            }
        }
    } else {
        autoNextHandTimer->stop();
    }

    if (isMyTurn && engine.myIdx < engine.players.size()) {
        const Player& p = engine.players[engine.myIdx];
        const int toCall = engine.currentHighestBet - p.currentBet;
        const int actualCallAmount = std::min(toCall, p.balance);

        // Центральная кнопка: CHECK или CALL
        if (toCall == 0) {
            btnCall->setText(getLocalizedText("CHECK", "CHECK"));
        } else {
            btnCall->setText(QString(getLocalizedText("CALL $%1", "CALL $%1")).arg(actualCallAmount));
        }

        // Правая кнопка (RAISE / ALL IN) и слайдер:
        // Если для колла требуется весь наш стек или больше — рейзить нечем, скрываем кнопку рейза!
        if (toCall >= p.balance) {
            btnRaise->setVisible(false);
            raiseSlider->setVisible(false);
            lblRaiseAmount->setVisible(false);
        } else {
            btnRaise->setVisible(true);

            int minR = engine.currentHighestBet + engine.minRaise;
            if (minR > p.balance + p.currentBet) minR = p.balance + p.currentBet;
            const int maxR = p.balance + p.currentBet;

            if (minR >= maxR) {
                // Стека хватает на колл, но не хватает на полный мин-рейз -> доступен только пуш ALL IN
                raiseSlider->setVisible(false);
                lblRaiseAmount->setVisible(false);
                btnRaise->setText(getLocalizedText("ALL IN", "ALL IN"));
            } else {
                // Полноценный выбор суммы рейза через слайдер
                raiseSlider->setVisible(true);
                lblRaiseAmount->setVisible(true);
                raiseSlider->setRange(minR, maxR);
                raiseSlider->setSingleStep(10);
                raiseSlider->setPageStep(10);
                raiseSlider->setValue(minR);
                lblRaiseAmount->setText(QString("$%1").arg(minR));
                btnRaise->setText(getLocalizedText("RAISE", "RAISE"));
            }
        }
    }

    if (!isLobby || engine.gameOver) {
        if (engine.myIdx < engine.players.size() && engine.players[engine.myIdx].holeCards.isEmpty() && !engine.gameOver) {
            lblStatus->setText(getLocalizedText("Вы зашли во время игры. Ожидание следующей раздачи...", "You joined mid-game. Waiting for next hand..."));
        } else if (!engine.statusMessage.isEmpty()) {
            lblStatus->setText(engine.statusMessage);
        }
    } else if (netManager && netManager->isHost) {
        lblStatus->setText(QString(getLocalizedText("ЛОББИ: %1/%2 игроков. Ожидание...", "LOBBY: %1/%2 players. Waiting...")).arg(activeClients + 1).arg(NetConfig::MAX_PLAYERS));
    }

    update();
}

void PokerWidget::resizeEvent(QResizeEvent* ev) {
    BaseTableWidget::resizeEvent(ev);
    const qreal s = getScale();

    const int btnW = qRound(110 * s);
    const int btnH = qRound(44 * s);
    const int sliderW = qRound(120 * s);
    const int btnY = height() - btnH - qRound(18 * s);

    btnFold->setGeometry(width() / 2 - btnW * 2 - qRound(15 * s), btnY, btnW, btnH);
    btnCall->setGeometry(width() / 2 - btnW - qRound(5 * s), btnY, btnW, btnH);
    raiseSlider->setGeometry(width() / 2 + qRound(5 * s), btnY, sliderW, qRound(18 * s));
    lblRaiseAmount->setGeometry(width() / 2 + qRound(5 * s), btnY + qRound(20 * s), sliderW, qRound(22 * s));
    btnRaise->setGeometry(width() / 2 + sliderW + qRound(15 * s), btnY, btnW, btnH);

    const QFont btnFont(font().family(), qMax(8, qRound(12 * s)), QFont::Bold);
    btnFold->setFont(btnFont);
    btnCall->setFont(btnFont);
    btnRaise->setFont(btnFont);
    lblRaiseAmount->setFont(btnFont);

    const int startNetW = qRound(260 * s);
    const int startNetH = qRound(50 * s);
    btnStartNetGame->setGeometry(width() / 2 - startNetW / 2, height() / 2 + qRound(40 * s), startNetW, startNetH);
    btnStartNetGame->setFont(btnFont);
}

void PokerWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    drawTableFelt(p);

    const qreal s = getScale();

    if (netManager && netManager->isNetworkGame && netManager->isLobby) {
        p.setPen(QColor(255, 215, 0));
        p.setFont(QFont(font().family(), qMax(12, qRound(22 * s)), QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, lblStatus->text());
        return;
    }

    if (!engine.players.isEmpty()) {
        p.setPen(QColor(252, 211, 77));
        p.setFont(QFont(font().family(), qMax(11, qRound(17 * s)), QFont::Bold));
        const int potY = height() / 2 - qRound(95 * s);
        p.drawText(QRect(0, potY, width(), qRound(28 * s)), Qt::AlignCenter, QString("POT: $%1").arg(engine.pot));

        const int cardW = qRound(80 * s);
        const int cardH = qRound(115 * s);
        const int stepX = qRound(88 * s);
        const int commStartX = width() / 2 - (5 * stepX) / 2;
        const int commY = height() / 2 - cardH / 2;

        for (int i = 0; i < 5; ++i) {
            const QRect cRect(commStartX + i * stepX, commY, cardW, cardH);
            if (i < engine.communityCards.size()) {
                drawCard(p, cRect, &engine.communityCards[i], true);
            } else {
                p.setPen(QPen(QColor(255, 255, 255, 40), 2, Qt::DashLine));
                p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(cRect, 6, 6);
            }
        }

        drawPlayers(p, cardW, cardH);
    }

    if (engine.gameOver) {
        drawGameOverBanner(p, engine.statusMessage);
    }
}

void PokerWidget::drawPlayers(QPainter& p, int cardW, int cardH) {
    const int numPlayers = engine.players.size();
    if (numPlayers <= 0) return; // Защита от деления на 0

    const qreal s = getScale();
    const int bottomOffset = qRound(115 * s);
    const int topOffset = qRound(80 * s);
    const QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), bottomOffset, topOffset);

    const int boxW = qRound(170 * s);
    const int boxH = qRound(50 * s);

    for (int i = 0; i < numPlayers; ++i) {
        const int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        const QPoint pos = seatPos[displayIdx];
        const auto& plr = engine.players[i];

        if (engine.currentTurnIdx == i && !engine.gameOver && !plr.hasFolded && !plr.isBankrupt && !plr.isDisconnected) {
            p.setPen(QPen(QColor(59, 130, 246, 220), qMax(2, qRound(3 * s))));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(pos.x() - boxW / 2 - 4, pos.y() - boxH / 2 - 4, boxW + 8, boxH + 8, 8, 8);
        }

        p.setBrush(QColor(15, 25, 35, 230));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.drawRoundedRect(pos.x() - boxW / 2, pos.y() - boxH / 2, boxW, boxH, 6, 6);

        if (engine.dealerIdx == i) {
            const int dSize = qRound(18 * s);
            p.setBrush(Qt::white);
            p.setPen(QPen(Qt::black, 1));
            p.drawEllipse(pos.x() - boxW / 2 - dSize / 2, pos.y() - dSize / 2, dSize, dSize);
            p.setPen(Qt::black);
            p.setFont(QFont(font().family(), qMax(7, qRound(10 * s)), QFont::Bold));
            p.drawText(QRect(pos.x() - boxW / 2 - dSize / 2, pos.y() - dSize / 2, dSize, dSize), Qt::AlignCenter, "D");
        }

        p.setPen(Qt::white);
        p.setFont(QFont(font().family(), qMax(8, qRound(11 * s)), QFont::Bold));
        const QString nameWithAvatar = getAvatarEmojiById(plr.avatar) + " " + plr.name;
        p.drawText(QRect(pos.x() - boxW / 2 + 8, pos.y() - boxH / 2 + 3, boxW - 16, boxH / 2), Qt::AlignLeft | Qt::AlignVCenter, nameWithAvatar);

        if (displayIdx == 0 && AppSettings::instance().getShowPokerHandHint() && !plr.hasFolded && !plr.holeCards.isEmpty() && engine.phase != PREFLOP) {
            QVector<Card> allCards = plr.holeCards;
            allCards.append(engine.communityCards);
            const HandValue hv = evaluate7Cards(allCards);

            p.setPen(QColor(251, 191, 36));
            p.setFont(QFont(font().family(), qMax(8, qRound(10 * s)), QFont::Bold));
            p.drawText(QRect(pos.x() - boxW, pos.y() + boxH / 2 + 2, boxW * 2, qRound(20 * s)), Qt::AlignCenter, QString("[%1]").arg(hv.name));
        }

        p.setPen(QColor(167, 243, 208));
        p.setFont(QFont(font().family(), qMax(8, qRound(10 * s)), QFont::Bold));
        p.drawText(QRect(pos.x() - boxW / 2 + 8, pos.y(), boxW - 16, boxH / 2), Qt::AlignLeft | Qt::AlignVCenter, QString("$%1").arg(plr.balance));

        if (plr.currentBet > 0) {
            p.setPen(QColor(253, 230, 138));
            p.drawText(QRect(pos.x() - boxW / 2 + 8, pos.y(), boxW - 16, boxH / 2), Qt::AlignRight | Qt::AlignVCenter, QString("Bet: %1").arg(plr.currentBet));
        }

        p.setFont(QFont(font().family(), qMax(8, qRound(10 * s)), QFont::Bold));
        if (plr.isDisconnected) {
            p.setPen(QColor(107, 114, 128));
            p.drawText(QRect(pos.x() - boxW / 2 + 8, pos.y() - boxH / 2 + 3, boxW - 16, boxH / 2), Qt::AlignRight | Qt::AlignVCenter, getLocalizedText("ВЫШЕЛ", "OFFLINE"));
        } else if (plr.isBankrupt) {
            p.setPen(QColor(156, 163, 175));
            p.drawText(QRect(pos.x() - boxW / 2 + 8, pos.y() - boxH / 2 + 3, boxW - 16, boxH / 2), Qt::AlignRight | Qt::AlignVCenter, getLocalizedText("РАЗОРЕН", "BANKRUPT"));
        } else if (plr.isAllIn) {
            p.setPen(QColor(248, 113, 113));
            p.drawText(QRect(pos.x() - boxW / 2 + 8, pos.y() - boxH / 2 + 3, boxW - 16, boxH / 2), Qt::AlignRight | Qt::AlignVCenter, getLocalizedText("ВА-БАНК", "ALL-IN"));
        } else if (plr.hasFolded) {
            p.setPen(QColor(156, 163, 175));
            p.drawText(QRect(pos.x() - boxW / 2 + 8, pos.y() - boxH / 2 + 3, boxW - 16, boxH / 2), Qt::AlignRight | Qt::AlignVCenter, getLocalizedText("СБРОС", "FOLD"));
        }

        if (!plr.hasFolded && !plr.isBankrupt && !plr.isDisconnected) {
            const int cardGap = qRound(8 * s);
            const int totalHoleW = plr.holeCards.size() * cardW + (plr.holeCards.size() - 1) * cardGap;
            const int cardsStartX = pos.x() - totalHoleW / 2;
            const int cardsY = (displayIdx == 0) ? pos.y() - cardH - qRound(32 * s) : pos.y() + boxH / 2 + qRound(6 * s);

            for (int c = 0; c < plr.holeCards.size(); ++c) {
                const bool faceUp = (i == engine.myIdx || engine.phase == SHOWDOWN || engine.gameOver);
                const QRect cRect(cardsStartX + c * (cardW + cardGap), cardsY, cardW, cardH);
                drawCard(p, cRect, &plr.holeCards[c], faceUp);
            }
        }
    }
}
