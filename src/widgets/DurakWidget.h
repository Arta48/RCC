#pragma once

#include "BaseTableWidget.h"
#include "engines/Durak.h"
#include "NetworkManager.h"

#include <QTimer>

/**
 * @brief Игровой стол для Подкидного Дурака.
 */
class DurakWidget : public BaseTableWidget {
    Q_OBJECT
public:
    DurakEngine     engine;
    NetworkManager* netManager = nullptr;

    int selectedHandCardIdx = -1;
    int hoveredHandCardIdx  = -1;

    QPushButton* btnPass;
    QPushButton* btnTake;
    QTimer*      aiTimer;

    explicit DurakWidget(NetworkManager* netMgr, QWidget* parent = nullptr);
    ~DurakWidget() override = default;

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
