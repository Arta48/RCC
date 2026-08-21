#include "UnoWidget.h"
#include "Audio.h"

#include <QStackedWidget>
#include <QHBoxLayout>

UnoWidget::UnoWidget(NetworkManager* netMgr, QWidget* parent)
: BaseTableWidget(parent), netManager(netMgr)
{
    btnDrawCard = new QPushButton(getLocalizedText("ВЗЯТЬ КАРТУ", "DRAW CARD"), this);
    btnPass     = new QPushButton(getLocalizedText("ПАС", "PASS"), this);
    btnUno      = new QPushButton(getLocalizedText("🔥 УНО!", "🔥 UNO!"), this);
    btnCatchUno = new QPushButton(getLocalizedText("⚡ ПОЙМАТЬ УНО!", "⚡ CATCH UNO!"), this);

    btnDrawCard->setStyleSheet("QPushButton { background: #2563EB; color: white; font-weight: bold; border-radius: 8px; padding: 6px; } QPushButton:hover { background: #3B82F6; }");
    btnPass->setStyleSheet("QPushButton { background: #64748B; color: white; font-weight: bold; border-radius: 8px; padding: 6px; } QPushButton:hover { background: #94A3B8; }");
    btnUno->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #DC2626, stop:1 #F59E0B); color: white; font-weight: 900; border-radius: 8px; border: 2px solid #FDE047; padding: 6px; } QPushButton:hover { background: #EF4444; }");
    btnCatchUno->setStyleSheet("QPushButton { background: #D97706; color: white; font-weight: bold; border-radius: 8px; border: 1px solid #FCD34D; padding: 6px; } QPushButton:hover { background: #F59E0B; }");

    btnDrawCard->setCursor(Qt::PointingHandCursor);
    btnPass->setCursor(Qt::PointingHandCursor);
    btnUno->setCursor(Qt::PointingHandCursor);
    btnCatchUno->setCursor(Qt::PointingHandCursor);

    colorPickerWidget = new QWidget(this);
    auto* cpLayout = new QHBoxLayout(colorPickerWidget);
    cpLayout->setContentsMargins(6, 6, 6, 6);
    cpLayout->setSpacing(8);
    colorPickerWidget->setStyleSheet("background: rgba(15, 23, 42, 0.95); border-radius: 22px; border: 2px solid rgba(251, 191, 36, 0.6);");

    const QString colStyles[] = { "#DC2626", "#EAB308", "#16A34A", "#2563EB" };
    const UnoColor colEnums[] = { UnoRed, UnoYellow, UnoGreen, UnoBlue };
    for (int i = 0; i < 4; ++i) {
        auto* btnCol = new QPushButton(colorPickerWidget);
        btnCol->setFixedSize(36, 36);
        btnCol->setCursor(Qt::PointingHandCursor);
        btnCol->setStyleSheet(QString("QPushButton { background: %1; border-radius: 18px; border: 2px solid white; } QPushButton:hover { border: 3px solid #FDE047; }").arg(colStyles[i]));
        UnoColor c = colEnums[i];
        connect(btnCol, &QPushButton::clicked, this, [this, c]() {
            chosenWildColor = c;
            colorPickerWidget->hide();
            if (selectedHandCardIdx >= 0) {
                const bool callUno = declaredUnoThisTurn;
                if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                    QJsonObject json; json["act"] = "PLAY"; json["cardIdx"] = selectedHandCardIdx; json["chosenColor"] = static_cast<int>(chosenWildColor); json["callUno"] = callUno;
                    netManager->sendJsonToServer(json);
                } else {
                    engine.playCard(engine.myIdx, selectedHandCardIdx, chosenWildColor, callUno);
                    if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
                }
                selectedHandCardIdx = -1;
                declaredUnoThisTurn = false;
                updateUI();
            }
        });
        cpLayout->addWidget(btnCol);
    }

    btnCancelColorPicker = new QPushButton("✕", colorPickerWidget);
    btnCancelColorPicker->setFixedSize(36, 36);
    btnCancelColorPicker->setCursor(Qt::PointingHandCursor);
    btnCancelColorPicker->setStyleSheet("QPushButton { background: #991B1B; color: white; font-weight: bold; font-size: 16px; border-radius: 18px; border: 2px solid #F87171; } QPushButton:hover { background: #DC2626; }");
    connect(btnCancelColorPicker, &QPushButton::clicked, this, [this]() {
        colorPickerWidget->hide();
        selectedHandCardIdx = -1;
        updateUI();
    });
    cpLayout->addWidget(btnCancelColorPicker);

    colorPickerWidget->hide();

    connect(btnUno, &QPushButton::clicked, this, [this]() {
        declaredUnoThisTurn = true;
        engine.declareUno(engine.myIdx);
        if (netManager && netManager->isNetworkGame && !netManager->isHost) {
            QJsonObject json; json["act"] = "UNO";
            netManager->sendJsonToServer(json);
        } else if (netManager && netManager->isNetworkGame && netManager->isHost) {
            broadcastNetState();
        }
        btnUno->hide();
    });

    connect(btnCatchUno, &QPushButton::clicked, this, [this]() {
        for (int i = 0; i < engine.players.size(); ++i) {
            if (i != engine.myIdx && engine.players[i].hand.size() == 1 && !engine.players[i].saidUno) {
                if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                    QJsonObject json; json["act"] = "CATCH"; json["targetIdx"] = i;
                    netManager->sendJsonToServer(json);
                } else {
                    engine.catchUno(engine.myIdx, i);
                    if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
                }
                break;
            }
        }
        updateUI();
    });

    connect(btnDrawCard, &QPushButton::clicked, this, [this]() {
        if (netManager && netManager->isNetworkGame && !netManager->isHost) {
            QJsonObject json; json["act"] = "DRAW";
            netManager->sendJsonToServer(json);
        } else {
            engine.drawCard(engine.myIdx);
            if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
        }
        updateUI();
    });

    connect(btnPass, &QPushButton::clicked, this, [this]() {
        if (netManager && netManager->isNetworkGame && !netManager->isHost) {
            QJsonObject json; json["act"] = "PASS";
            netManager->sendJsonToServer(json);
        } else {
            engine.passTurn(engine.myIdx);
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
    connect(aiTimer, &QTimer::timeout, this, &UnoWidget::handleAiLogic);
    connect(&engine, &UnoEngine::stateChanged, this, &UnoWidget::updateUI);

    // =========================================================================
    // ТАЙМЕР АНИМАЦИИ ВРАЩЕНИЯ СТРЕЛОК УНО
    // =========================================================================
    arrowAnimTimer = new QTimer(this);
    animElapsedTimer.start();
    connect(arrowAnimTimer, &QTimer::timeout, this, [this]() {
        // Проверяем, что виджет виден и игра не окончена
        if (!engine.gameOver && isVisible() && parentWidget() && static_cast<QStackedWidget*>(parentWidget())->currentWidget() == this) {
            qreal dt = animElapsedTimer.restart() / 1000.0;
            if (dt > 0.1) dt = 0.1;

            const qreal ROTATION_SPEED_DEG_PER_SEC = 60.0;
            arrowAnimAngle += engine.direction * ROTATION_SPEED_DEG_PER_SEC * dt;

            if (arrowAnimAngle >= 360.0) arrowAnimAngle -= 360.0;
            if (arrowAnimAngle < 0.0)    arrowAnimAngle += 360.0;

            // Частичная перерисовка строго в области вращающихся стрелок
            update(getArrowBoundingRect());
        } else {
            animElapsedTimer.restart();
        }
    });
    arrowAnimTimer->start(16); // 60 FPS
}

QRect UnoWidget::getArrowBoundingRect() const {
    const qreal s = getScale();
    const QPoint center(width() / 2, height() / 2 - qRound(25 * s));
    const int radius = qRound(115 * s);
    return QRect(center.x() - radius, center.y() - radius, radius * 2, radius * 2);
}

void UnoWidget::startSingleGame(int botCount) {
    if (netManager) netManager->disconnectAll();
    engine.initGame(botCount);
    updateUI();
    aiTimer->start(800);
}

void UnoWidget::handleAiLogic() {
    if (parentWidget() && static_cast<QStackedWidget*>(parentWidget())->currentIndex() != 4) return;
    if (engine.isProcessingMove) return;

    if (!engine.gameOver && (!netManager || !netManager->isNetworkGame || netManager->isHost)) {
        if (engine.makeAiMove()) {
            if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
            updateUI();
        }
    }
}

void UnoWidget::updateUI() {
    const bool isMyTurn = (engine.currentTurnIdx == engine.myIdx && !engine.gameOver);
    const bool hasMove = (engine.myIdx >= 0 && engine.myIdx < engine.players.size()) && engine.hasPlayableCard(engine.myIdx);

    if (engine.accumulatedPenalty > 0) {
        btnDrawCard->setText(QString(getLocalizedText("ВЗЯТЬ ШТРАФ (+%1)", "TAKE PENALTY (+%1)")).arg(engine.accumulatedPenalty));
        btnDrawCard->setVisible(isMyTurn);
        btnPass->setVisible(false);
    } else {
        btnDrawCard->setText(getLocalizedText("ВЗЯТЬ КАРТУ", "DRAW CARD"));
        btnDrawCard->setVisible(isMyTurn && !hasMove && (!engine.hasDrawnThisTurn || engine.drawMode == UnoDrawMode::DrawUntilMatch));
        btnPass->setVisible(isMyTurn && engine.hasDrawnThisTurn && engine.drawMode == UnoDrawMode::DrawOne);
    }

    if (engine.myIdx >= 0 && engine.myIdx < engine.players.size()) {
        const int handCount = engine.players[engine.myIdx].hand.size();
        const bool isVulnerable = (engine.unoVulnerablePlayerIdx == engine.myIdx);
        btnUno->setVisible((isMyTurn && handCount <= 2 && hasMove && !engine.players[engine.myIdx].saidUno) || isVulnerable);
    } else {
        btnUno->setVisible(false);
    }

    bool canCatch = false;
    for (int i = 0; i < engine.players.size(); ++i) {
        if (i != engine.myIdx && (engine.unoVulnerablePlayerIdx == i || (engine.players[i].hand.size() == 1 && !engine.players[i].saidUno))) {
            canCatch = true;
            break;
        }
    }
    btnCatchUno->setVisible(canCatch && !engine.gameOver);

    const int activeClients = netManager ? netManager->getActiveClientCount() : 0;
    const bool isHostOrSolo = (!netManager || !netManager->isNetworkGame || netManager->isHost);
    const bool isError = engine.statusMessage.contains(getLocalizedText("Ошибка", "Error")) || engine.statusMessage.contains(getLocalizedText("потеряна", "lost"));
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
        colorPickerWidget->hide();
    } else {
        static const QString colNames[] = {
            getLocalizedText("Красный", "Red"),
            getLocalizedText("Жёлтый", "Yellow"),
            getLocalizedText("Зелёный", "Green"),
            getLocalizedText("Синий", "Blue")
        };
        const int safeCol = qBound(0, static_cast<int>(engine.currentColor), 3);
        const QString colorTxt = colNames[safeCol];
        const QString penaltySuffix = (engine.accumulatedPenalty > 0) ? QString(getLocalizedText(" [ШТРАФ: +%1]", " [PENALTY: +%1]")).arg(engine.accumulatedPenalty) : "";

        if (engine.currentTurnIdx == engine.myIdx) {
            lblStatus->setText(QString(getLocalizedText("Ваш ход! Цвет: %1%2", "Your turn! Color: %1%2")).arg(colorTxt, penaltySuffix));
        } else if (engine.currentTurnIdx >= 0 && engine.currentTurnIdx < engine.players.size()) {
            lblStatus->setText(QString(getLocalizedText("Ход игрока %1 (Цвет: %2)%3", "%1's turn (Color: %2)%3")).arg(engine.players[engine.currentTurnIdx].name, colorTxt, penaltySuffix));
        } else {
            lblStatus->setText(engine.statusMessage);
        }
    }

    update();
}

void UnoWidget::processNetAction(int senderId, const QJsonObject& json) {
    if (!netManager || !netManager->isHost) return;
    if (senderId < 0 || senderId >= NetConfig::MAX_PLAYERS) return;

    if (senderId >= engine.players.size()) {
        while (engine.players.size() <= senderId) {
            UnoPlayer p;
            p.id = engine.players.size();
            p.name = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].name : QString(getLocalizedText("Игрок %1", "Player %1")).arg(p.id + 1);
            p.avatar = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].avatar : 0;
            p.isBot = false;
            engine.players.append(p);
        }
    }

    const QString act = json["act"].toString();
    const auto& hand = engine.players[senderId].hand;

    // Строгая валидация сетевых команд Уно
    if (act == "PLAY") {
        if (senderId != engine.currentTurnIdx || engine.gameOver) return;

        const int cardIdx = json["cardIdx"].toInt(-1);
        const int rawColor = json["chosenColor"].toInt(0);
        const UnoColor col = static_cast<UnoColor>(qBound(0, rawColor, 3));
        const bool callUno = json["callUno"].toBool(false);

        if (cardIdx >= 0 && cardIdx < hand.size() && engine.canPlayCard(hand[cardIdx])) {
            engine.playCard(senderId, cardIdx, col, callUno);
        }
    } else if (act == "DRAW") {
        if (senderId == engine.currentTurnIdx && !engine.gameOver) {
            engine.drawCard(senderId);
        }
    } else if (act == "PASS") {
        if (senderId == engine.currentTurnIdx && !engine.gameOver) {
            engine.passTurn(senderId);
        }
    } else if (act == "UNO") {
        engine.declareUno(senderId);
    } else if (act == "CATCH") {
        const int targetIdx = json["targetIdx"].toInt(-1);
        if (targetIdx >= 0 && targetIdx < engine.players.size() && targetIdx != senderId) {
            engine.catchUno(senderId, targetIdx);
        }
    }
    broadcastNetState();
}

void UnoWidget::broadcastNetState() {
    if (!netManager || !netManager->isHost) return;
    for (int i = 0; i < netManager->clientSockets.size(); ++i) {
        auto* socket = netManager->clientSockets[i];
        const int targetPlayerId = i + 1;
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            QJsonObject json = engine.toJson(targetPlayerId);
            json["isLobby"]  = false;
            json["gameType"] = 3;
            netManager->sendJsonToClient(i, json);
        }
    }
    updateUI();
}

void UnoWidget::resizeEvent(QResizeEvent* ev) {
    BaseTableWidget::resizeEvent(ev);
    const qreal s = getScale();

    const int btnW = qRound(140 * s);
    const int btnH = qRound(44 * s);
    const int unoW = qRound(130 * s);
    const int btnY = height() - btnH - qRound(18 * s);

    btnDrawCard->setGeometry(width() - btnW - qRound(20 * s), btnY, btnW, btnH);
    btnPass->setGeometry(width() - btnW - qRound(20 * s), btnY, btnW, btnH);
    btnUno->setGeometry(width() - btnW - unoW - qRound(30 * s), btnY, unoW, btnH);
    btnCatchUno->setGeometry(width() - btnW - unoW - qRound(30 * s), btnY, unoW, btnH);

    const QFont btnFont(font().family(), qMax(8, qRound(12 * s)), QFont::Bold);
    btnDrawCard->setFont(btnFont);
    btnPass->setFont(btnFont);
    btnUno->setFont(btnFont);
    btnCatchUno->setFont(btnFont);

    const int cpW = qRound(240 * s);
    const int cpH = qRound(46 * s);
    colorPickerWidget->setGeometry(width() / 2 - cpW / 2, height() - cpH - qRound(55 * s), cpW, cpH);
    colorPickerWidget->setStyleSheet(QString(
        "background: rgba(15, 23, 42, 0.95); "
        "border-radius: %1px; "
        "border: 2px solid rgba(251, 191, 36, 0.6);"
    ).arg(cpH / 2));

    const int circleSize = qRound(32 * s);
    const int circleRadius = circleSize / 2;

    const QString colStyles[] = { "#DC2626", "#EAB308", "#16A34A", "#2563EB" };
    QList<QPushButton*> colorButtons = colorPickerWidget->findChildren<QPushButton*>();
    for (int i = 0; i < colorButtons.size(); ++i) {
        auto* btn = colorButtons[i];
        btn->setFixedSize(circleSize, circleSize);
        if (i < 4) {
            btn->setStyleSheet(QString(
                "QPushButton { background: %1; border-radius: %2px; border: 2px solid white; } "
                "QPushButton:hover { border: 3px solid #FDE047; }"
            ).arg(colStyles[i]).arg(circleRadius));
        } else {
            btn->setFont(QFont(font().family(), qMax(8, qRound(13 * s)), QFont::Bold));
            btn->setStyleSheet(QString(
                "QPushButton { background: #991B1B; color: white; font-weight: bold; border-radius: %1px; border: 2px solid #F87171; } "
                "QPushButton:hover { background: #DC2626; }"
            ).arg(circleRadius));
        }
    }

    drawDeckRect = QRect(getSafeLeftMargin() + qRound(15 * s), qRound(100 * s), qRound(80 * s), qRound(115 * s));
}

void UnoWidget::drawUnoCard(QPainter& p, const QRect& rect, const UnoCard* card, bool faceUp, bool selected) {
    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 120));
    p.drawRoundedRect(rect.translated(3, 4), 8, 8);

    QPainterPath path;
    path.addRoundedRect(rect, 8, 8);

    if (!faceUp || !card) {
        QColor shirtBg;
        switch (AppSettings::instance().getCardShirt()) {
            case CardShirtStyle::RedVelvet:   shirtBg = QColor(136, 19, 19); break;
            case CardShirtStyle::GoldRoyal:   shirtBg = QColor(140, 100, 10); break;
            case CardShirtStyle::DarkPattern: shirtBg = QColor(18, 18, 20); break;
            default:                          shirtBg = QColor(15, 23, 42); break;
        }

        p.fillPath(path, shirtBg);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::white, 1.5));
        p.drawPath(path);

        p.save();
        p.setClipPath(path);

        p.save();
        p.translate(rect.center());
        p.rotate(34);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(220, 38, 38));
        const QRectF backOval(-rect.width() * 0.33, -rect.height() * 0.49, rect.width() * 0.74, rect.height() * 1.02);
        p.drawEllipse(backOval);
        p.restore();

        p.save();
        p.translate(rect.center());
        p.rotate(-14);

        const int backUnoFont = qMax(8, qRound(rect.height() * 0.16));
        QFont unoFont(p.font().family(), backUnoFont, QFont::Black);
        unoFont.setItalic(true);
        p.setFont(unoFont);

        const QRectF textRect(-rect.width() / 2.0, -rect.height() / 2.0, rect.width(), rect.height());
        p.setPen(QColor(0, 0, 0, 180));
        p.drawText(textRect.translated(1.5, 1.5), Qt::AlignCenter, "UNO");

        p.setPen(QColor(253, 224, 71));
        p.drawText(textRect, Qt::AlignCenter, "UNO");
        p.restore();

        p.restore();
        p.restore();
        return;
    }

    QColor cardBg;
    switch (card->color) {
        case UnoRed:    cardBg = QColor(220, 38, 38); break;
        case UnoYellow: cardBg = QColor(234, 179, 8); break;
        case UnoGreen:  cardBg = QColor(22, 163, 74); break;
        case UnoBlue:   cardBg = QColor(37, 99, 235); break;
        default:        cardBg = QColor(15, 23, 42); break;
    }

    p.fillPath(path, selected ? cardBg.lighter(130) : cardBg);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(selected ? QColor(254, 240, 138) : Qt::white, selected ? 3 : 1.5));
    p.drawPath(path);

    p.save();
    p.setClipPath(path);

    const QRectF ovalRect(-rect.width() * 0.33, -rect.height() * 0.49, rect.width() * 0.74, rect.height() * 1.02);

    if (card->color == UnoWild) {
        p.setPen(Qt::NoPen);

        p.save();
        p.setClipRect(QRectF(rect.left(), rect.top(), rect.width(), rect.height() / 2.0));
        p.translate(rect.center());
        p.rotate(34);
        p.setBrush(QColor(220, 38, 38)); p.drawPie(ovalRect, 90 * 16, 180 * 16);
        p.setBrush(QColor(37, 99, 235));  p.drawPie(ovalRect, -90 * 16, 180 * 16);
        p.restore();

        p.save();
        p.setClipRect(QRectF(rect.left(), rect.center().y(), rect.width(), rect.height() / 2.0));
        p.translate(rect.center());
        p.rotate(34);
        p.setBrush(QColor(234, 179, 8));  p.drawPie(ovalRect, 90 * 16, 180 * 16);
        p.setBrush(QColor(22, 163, 74));  p.drawPie(ovalRect, -90 * 16, 180 * 16);
        p.restore();

        p.save();
        p.translate(rect.center());
        p.rotate(34);
        p.setPen(QPen(Qt::white, 2.5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(ovalRect);
        p.restore();
    } else {
        p.save();
        p.translate(rect.center());
        p.rotate(34);
        p.setPen(QPen(Qt::white, 2.5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(ovalRect);
        p.restore();
    }

    if (card->color == UnoWild) {
        if (card->value == UnoWildDrawFour) {
            const int wild4Font = qMax(12, qRound(rect.height() * 0.26));
            QFont centerFont(p.font().family(), wild4Font, QFont::Black);
            centerFont.setItalic(true);
            p.setFont(centerFont);
            p.setPen(QColor(0, 0, 0, 100));
            p.drawText(rect.translated(2, 2), Qt::AlignCenter, "+4");
            p.setPen(Qt::white);
            p.drawText(rect, Qt::AlignCenter, "+4");
        }
    } else {
        QString centerTxt;
        if (card->value <= UnoNine) centerTxt = QString::number(card->value);
        else if (card->value == UnoSkip) centerTxt = "⊘";
        else if (card->value == UnoReverse) centerTxt = "⇄";
        else if (card->value == UnoDrawTwo) centerTxt = "+2";

        const int centerFontSize = qMax(10, qRound(rect.height() * (centerTxt.length() > 1 ? 0.22 : 0.30)));
        QFont centerFont(p.font().family(), centerFontSize, QFont::Black);
        centerFont.setItalic(true);
        p.setFont(centerFont);

        p.setPen(QColor(0, 0, 0, 80));
        p.drawText(rect.translated(2, 2), Qt::AlignCenter, centerTxt);

        p.setPen(Qt::white);
        p.drawText(rect, Qt::AlignCenter, centerTxt);
    }

    p.restore();

    const int cornerFontSize = qMax(7, qRound(rect.height() * 0.11));

    if (card->color == UnoWild && card->value == UnoWildCard) {
        auto drawMiniWildOval = [&](const QPointF& pt) {
            const qreal mw = rect.width() * 0.11;
            const qreal mh = rect.height() * 0.13;
            const QRectF miniRect(-mw / 2.0, -mh / 2.0, mw, mh);
            p.setPen(Qt::NoPen);

            p.save();
            p.setClipRect(QRectF(pt.x() - mw, pt.y() - mh, mw * 2, mh));
            p.translate(pt);
            p.rotate(34);
            p.setBrush(QColor(220, 38, 38)); p.drawPie(miniRect, 90 * 16, 180 * 16);
            p.setBrush(QColor(37, 99, 235));  p.drawPie(miniRect, -90 * 16, 180 * 16);
            p.restore();

            p.save();
            p.setClipRect(QRectF(pt.x() - mw, pt.y(), mw * 2, mh));
            p.translate(pt);
            p.rotate(34);
            p.setBrush(QColor(234, 179, 8));  p.drawPie(miniRect, 90 * 16, 180 * 16);
            p.setBrush(QColor(22, 163, 74));  p.drawPie(miniRect, -90 * 16, 180 * 16);
            p.restore();

            p.save();
            p.translate(pt);
            p.rotate(34);
            p.setPen(QPen(Qt::white, 1.2));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(miniRect);
            p.restore();
        };

        drawMiniWildOval(QPointF(rect.left() + rect.width() * 0.14, rect.top() + rect.height() * 0.13));
        drawMiniWildOval(QPointF(rect.right() - rect.width() * 0.14, rect.bottom() - rect.height() * 0.13));
    } else {
        QString cornerTxt;
        if (card->value <= UnoNine) cornerTxt = QString::number(card->value);
        else if (card->value == UnoSkip) cornerTxt = "⊘";
        else if (card->value == UnoReverse) cornerTxt = "⇄";
        else if (card->value == UnoDrawTwo) cornerTxt = "+2";
        else if (card->value == UnoWildDrawFour) cornerTxt = "+4";

        QFont cornerFont(p.font().family(), cornerFontSize, QFont::Bold);
        cornerFont.setItalic(true);
        p.setFont(cornerFont);

        p.setPen(QColor(0, 0, 0, 100));
        p.drawText(rect.adjusted(5, 3, -3, -3), Qt::AlignTop | Qt::AlignLeft, cornerTxt);
        p.setPen(Qt::white);
        p.drawText(rect.adjusted(4, 2, -3, -3), Qt::AlignTop | Qt::AlignLeft, cornerTxt);

        p.save();
        p.translate(rect.center());
        p.rotate(180);
        const QRectF localRect(-rect.width() / 2.0, -rect.height() / 2.0, rect.width(), rect.height());
        p.setPen(QColor(0, 0, 0, 100));
        p.drawText(localRect.adjusted(5, 3, -3, -3), Qt::AlignTop | Qt::AlignLeft, cornerTxt);
        p.setPen(Qt::white);
        p.drawText(localRect.adjusted(4, 2, -3, -3), Qt::AlignTop | Qt::AlignLeft, cornerTxt);
        p.restore();
    }

    p.restore();
}

void UnoWidget::mouseMoveEvent(QMouseEvent* ev) {
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

void UnoWidget::mousePressEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty()) return;

    if (drawDeckRect.contains(ev->pos()) && engine.currentTurnIdx == engine.myIdx) {
        if (colorPickerWidget->isVisible()) {
            colorPickerWidget->hide();
            selectedHandCardIdx = -1;
        }
        if (btnDrawCard->isVisible()) {
            btnDrawCard->click();
        }
        return;
    }

    if (engine.currentTurnIdx != engine.myIdx) return;

    const auto& myHand = engine.players[engine.myIdx].hand;
    const qreal s = getScale();

    const int cardW = qRound(80 * s);
    const int cardH = qRound(115 * s);
    const int handY = height() - cardH - qRound(110 * s);
    const int stepX = qMin(qRound(50 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
    const int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    bool clickedOnCard = false;

    for (int i = myHand.size() - 1; i >= 0; --i) {
        const int offsetY = (i == selectedHandCardIdx) ? qRound(-25 * s) : ((i == hoveredHandCardIdx) ? qRound(-12 * s) : 0);
        if (QRect(startX + i * stepX, handY + offsetY, cardW, cardH).contains(ev->pos())) {
            clickedOnCard = true;
            if (!engine.canPlayCard(myHand[i])) return;

            if (myHand[i].color == UnoWild) {
                selectedHandCardIdx = i;
                colorPickerWidget->show();
                update();
                return;
            }

            if (colorPickerWidget->isVisible()) {
                colorPickerWidget->hide();
            }

            selectedHandCardIdx = i;
            const bool callUno = declaredUnoThisTurn;
            if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                QJsonObject json; json["act"] = "PLAY"; json["cardIdx"] = selectedHandCardIdx; json["chosenColor"] = static_cast<int>(myHand[i].color); json["callUno"] = callUno;
                netManager->sendJsonToServer(json);
            } else {
                engine.playCard(engine.myIdx, selectedHandCardIdx, myHand[i].color, callUno);
                if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
            }
            selectedHandCardIdx = -1;
            declaredUnoThisTurn = false;
            updateUI();
            return;
        }
    }

    if (!clickedOnCard && colorPickerWidget->isVisible() && !colorPickerWidget->geometry().contains(ev->pos())) {
        colorPickerWidget->hide();
        selectedHandCardIdx = -1;
        update();
    }
}

void UnoWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawTableFelt(p);

    const qreal s = getScale();

    if (!engine.players.isEmpty()) {
        const int cardW = qRound(80 * s);
        const int cardH = qRound(115 * s);

        const int deckX = getSafeLeftMargin() + qRound(15 * s);
        const int deckY = qRound(100 * s);
        drawDeckRect = QRect(deckX, deckY, cardW, cardH);

        if (engine.deck.size() > 1) drawUnoCard(p, QRect(deckX + 4, deckY + 4, cardW, cardH), nullptr, false);
        if (engine.deck.size() > 5) drawUnoCard(p, QRect(deckX + 2, deckY + 2, cardW, cardH), nullptr, false);
        drawUnoCard(p, drawDeckRect, nullptr, false);

        const QRect badgeRect(deckX - 5, deckY + cardH + qRound(8 * s), cardW + 10, qRound(22 * s));
        p.setBrush(QColor(15, 23, 42, 220));
        p.setPen(QPen(QColor(251, 191, 36, 180), 1));
        p.drawRoundedRect(badgeRect, 6, 6);
        p.setFont(QFont(font().family(), qMax(8, qRound(10 * s)), QFont::Bold));
        p.setPen(Qt::white);
        p.drawText(badgeRect, Qt::AlignCenter, QString(getLocalizedText("Карт: %1", "Cards: %1")).arg(engine.deck.size()));

        drawCenterDiscard(p, cardW, cardH);
        drawPlayers(p, cardW, cardH);

        if (engine.myIdx < engine.players.size()) {
            const auto& myHand = engine.players[engine.myIdx].hand;
            const int handY = height() - cardH - qRound(110 * s);
            const int stepHandX = qMin(qRound(50 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
            const int startX = (width() - (myHand.size() * stepHandX + (cardW - stepHandX))) / 2;
            for (int i = 0; i < myHand.size(); ++i) {
                const bool isSelected = (i == selectedHandCardIdx);
                const bool isHovered  = (i == hoveredHandCardIdx);
                const int offsetY     = isSelected ? qRound(-25 * s) : (isHovered ? qRound(-12 * s) : 0);
                drawUnoCard(p, QRect(startX + i * stepHandX, handY + offsetY, cardW, cardH), &myHand[i], true, isSelected);
            }
        }
    }

    if (engine.gameOver) {
        drawGameOverBanner(p, engine.statusMessage);
    }
}

void UnoWidget::drawCenterDiscard(QPainter& p, int cardW, int cardH) {
    const qreal s = getScale();
    const QPoint center(width() / 2, height() / 2 - qRound(25 * s));

    const QColor arrowColors[] = { QColor(220, 38, 38), QColor(234, 179, 8), QColor(22, 163, 74), QColor(37, 99, 235) };
    const int safeCol = qBound(0, static_cast<int>(engine.currentColor), 3); // Защита от UnoWild (4)
    const QColor curCol = arrowColors[safeCol];

    const int arrowRadius = qRound(105 * s);
    p.setPen(QPen(QColor(curCol.red(), curCol.green(), curCol.blue(), 70), qMax(2, qRound(3 * s)), Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(center, arrowRadius, arrowRadius);

    p.save();
    p.translate(center);
    p.setPen(QPen(curCol, 2));
    p.setBrush(curCol);

    for (int a = 0; a < 360; a += 180) {
        p.save();
        p.rotate(a + arrowAnimAngle);
        p.translate(0, -arrowRadius);
        QPolygonF arrow;
        const qreal aS = s * 9.0;
        if (engine.direction == 1) arrow << QPointF(-aS, -aS * 0.8) << QPointF(aS, 0) << QPointF(-aS, aS * 0.8);
        else                       arrow << QPointF(aS, -aS * 0.8) << QPointF(-aS, 0) << QPointF(aS, aS * 0.8);
        p.drawPolygon(arrow);
        p.restore();
    }
    p.restore();

    const int discardX = center.x() - cardW / 2;
    const int discardY = center.y() - cardH / 2;

    if (engine.discardPile.size() > 1) {
        p.save();
        p.translate(center.x(), center.y());
        p.rotate(-9);
        drawUnoCard(p, QRect(-cardW / 2, -cardH / 2, cardW, cardH), &engine.discardPile[engine.discardPile.size() - 2], true);
        p.restore();
    }

    if (!engine.discardPile.isEmpty()) {
        drawUnoCard(p, QRect(discardX, discardY, cardW, cardH), &engine.discardPile.last(), true);
    }
}

void UnoWidget::drawPlayers(QPainter& p, int cardW, int cardH) {
    const int numPlayers = engine.players.size();
    if (numPlayers <= 0) return; // Защита от деления на 0

    const qreal s = getScale();
    const QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), qRound(75 * s), qRound(80 * s));

    const int boxW = qRound(150 * s);
    const int boxH = qRound(42 * s);

    for (int i = 0; i < numPlayers; ++i) {
        const int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        const QPoint pos = seatPos[displayIdx];
        const auto& opp = engine.players[i];
        const int handSize = opp.hand.size();

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
        const QString nameWithAvatar = getAvatarEmojiById(opp.avatar) + " " + opp.name;
        p.drawText(QRect(pos.x() - boxW / 2 + 5, pos.y() - boxH / 2, boxW - 10, boxH), Qt::AlignVCenter | Qt::AlignLeft, nameWithAvatar);

        const int badgeSize = qRound(38 * s);
        const QRect badgeRect(pos.x() + boxW / 2 + qRound(5 * s), pos.y() - badgeSize / 2, badgeSize, badgeSize);
        const bool isUno = (handSize == 1);
        p.setBrush(isUno ? QColor(220, 38, 38) : QColor(30, 41, 59, 240));
        p.setPen(QPen(isUno ? QColor(254, 240, 138) : QColor(255, 255, 255, 60), isUno ? 2 : 1));
        p.drawRoundedRect(badgeRect, 6, 6);

        p.setFont(QFont(font().family(), qMax(8, qRound((isUno ? 9 : 11) * s)), QFont::Bold));
        p.setPen(Qt::white);
        p.drawText(badgeRect, Qt::AlignCenter, isUno ? "UNO!" : QString("x%1").arg(handSize));

        if (displayIdx != 0) {
            const int oppStep = qRound(15 * s);
            const int oppW = cardW - qRound(25 * s);
            const int oppH = cardH - qRound(35 * s);
            const int startX = pos.x() - (handSize * oppStep + (oppW - oppStep)) / 2;
            for (int c = 0; c < handSize; ++c) {
                drawUnoCard(p, QRect(startX + c * oppStep, pos.y() + boxH / 2 + qRound(5 * s), oppW, oppH), nullptr, false);
            }
        }
    }
}
