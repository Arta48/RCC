#pragma once

#include "BaseTableWidget.h"
#include "engines/Uno.h"
#include "NetworkManager.h"

#include <QTimer>
#include <QElapsedTimer>

/**
 * @brief Игровой стол для игры Уно с поддержкой цветных спецкарт и анимацией направления хода.
 */
class UnoWidget : public BaseTableWidget {
    Q_OBJECT
public:
    UnoEngine       engine;
    NetworkManager* netManager = nullptr;

    int selectedHandCardIdx = -1;
    int hoveredHandCardIdx  = -1;
    UnoColor chosenWildColor = UnoRed;
    bool declaredUnoThisTurn = false;

    QPushButton* btnDrawCard;
    QPushButton* btnPass;
    QPushButton* btnUno;
    QPushButton* btnCatchUno;
    QPushButton* btnCancelColorPicker;
    QWidget*     colorPickerWidget;
    QTimer*      aiTimer;

    QRect drawDeckRect;

    explicit UnoWidget(NetworkManager* netMgr, QWidget* parent = nullptr);
    ~UnoWidget() override = default;

    void startSingleGame(int botCount);
    void handleAiLogic();
    void updateUI();
    void processNetAction(int senderId, const QJsonObject& json);
    void broadcastNetState();

    static void drawUnoCard(QPainter& p, const QRect& rect, const UnoCard* card, bool faceUp, bool selected = false);

    /**
     * @brief Вычисляет точные экранные границы вращающейся стрелки направления для оптимизированного update().
     */
    QRect getArrowBoundingRect() const;

    qreal         arrowAnimAngle = 0.0;
    QTimer*       arrowAnimTimer = nullptr;
    QElapsedTimer animElapsedTimer;

protected:
    void resizeEvent(QResizeEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void paintEvent(QPaintEvent* ev) override;

private:
    void drawPlayers(QPainter& p, int cardW, int cardH);
    void drawCenterDiscard(QPainter& p, int cardW, int cardH);
};
