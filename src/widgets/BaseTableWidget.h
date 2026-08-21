#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QVector>
#include <QPoint>
#include <functional>

#include "engines/Poker.h"
#include "AppSettings.h"

/**
 * @brief Базовый абстрактный класс игрового стола, предоставляющий единый рендеринг сукна, карт и баннеров.
 */
class BaseTableWidget : public QWidget {
    Q_OBJECT
public:
    QPushButton* btnBackMenu;
    QLabel*      lblStatus;
    QPushButton* btnNextHand;
    QPushButton* btnSettings;
    QPushButton* btnRules;

    std::function<void()> onBackToMenuCallback;
    std::function<void()> onReturnToLobbyCallback;
    std::function<void()> onOpenSettingsCallback;
    std::function<void()> onOpenRulesCallback;

    explicit BaseTableWidget(QWidget* parent = nullptr);
    ~BaseTableWidget() override = default;

    /**
     * @brief Вычисляет глобальный масштабный коэффициент для адаптивной верстки.
     */
    qreal getScale() const {
        const qreal scaleW = width() / 1280.0;
        const qreal scaleH = height() / 720.0;
        return std::clamp(std::min(scaleW, scaleH), 0.45, 2.5);
    }

    /**
     * @brief Возвращает безопасный левый отступ (Safe Area) для мобильных устройств с вырезом под камеру.
     */
    int getSafeLeftMargin() const {
        #if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        return qMax(15, qRound(35 * getScale()));
        #else
        return qRound(15 * getScale());
        #endif
    }

protected:
    void drawTableFelt(QPainter& p);
    void drawGameOverBanner(QPainter& p, const QString& message);
    static void drawCard(QPainter& p, const QRect& rect, const Card* card, bool faceUp, bool selected = false);
    static QVector<QPoint> getSeatPositions(int numPlayers, int width, int height, int bottomYOffset = 75, int topYOffset = 80);

    void resizeEvent(QResizeEvent* ev) override;
};
