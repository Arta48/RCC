#pragma once

#include "BaseTableWidget.h"
#include "engines/Poker.h"
#include "NetworkManager.h"

#include <QSlider>
#include <QTimer>

/**
 * @brief Игровой стол для Техасского Холдема (Одиночный режим против ИИ и Мультиплеер).
 */
class PokerWidget : public BaseTableWidget {
    Q_OBJECT
public:
    PokerEngine     engine;
    NetworkManager* netManager = nullptr;

    QPushButton* btnFold;
    QPushButton* btnCall;
    QPushButton* btnRaise;
    QSlider*     raiseSlider;
    QLabel*      lblRaiseAmount;

    QPushButton* btnStartNetGame;
    QTimer*      aiTimer;
    QTimer*      autoNextHandTimer;

    explicit PokerWidget(NetworkManager* netMgr, QWidget* parent = nullptr);
    ~PokerWidget() override = default;

    void startSingleGame(int botCount);
    void onPlayerAction(const QString& action, int raiseTotal = 0);
    void handleAiLogic();
    void updateUI();
    void processNetAction(int senderId, const QJsonObject& json);
    void broadcastNetState();

protected:
    void resizeEvent(QResizeEvent* ev) override;
    void paintEvent(QPaintEvent* ev) override;

private:
    void drawPlayers(QPainter& p, int cardW, int cardH);
};
