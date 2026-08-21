#pragma once

#include "BaseTableWidget.h"
#include "engines/Kozel.h"
#include "NetworkManager.h"

#include <QTimer>

/**
 * @brief Игровой стол для традиционной игры Козёл.
 */
class KozelWidget : public BaseTableWidget {
    Q_OBJECT
public:
    KozelEngine     engine;
    NetworkManager* netManager = nullptr;

    QVector<int> selectedHandCardIndices;
    int          hoveredHandCardIdx = -1;

    QPushButton* btnPlayCards;
    QTimer*      aiTimer;

    explicit KozelWidget(NetworkManager* netMgr, QWidget* parent = nullptr);
    ~KozelWidget() override = default;

    void startSingleGame(int botCount);
    void handleAiLogic();
    void updateUI();

    void processNetAction(int senderId, const QJsonObject& json);
    void broadcastNetState();

protected:
    void resizeEvent(QResizeEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void paintEvent(QPaintEvent* ev) override;

private:
    void drawPlayers(QPainter& p, int cardW, int cardH);
};
