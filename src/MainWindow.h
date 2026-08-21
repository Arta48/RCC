#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QFrame>
#include <QSlider>
#include <QDialog>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QShortcut>
#include <QTimer>
#include <QElapsedTimer>
#include <functional>

#include "NetworkManager.h"
#include "Poker.h"
#include "Durak.h"
#include "Kozel.h"
#include "Uno.h"
#include "AppSettings.h"

/**
 * @brief Базовый класс стола с общей визуализацией сукна, карт и баннеров.
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

    qreal getScale() const {
        qreal scaleW = width() / 1280.0;
        qreal scaleH = height() / 720.0;
        return std::clamp(std::min(scaleW, scaleH), 0.45, 2.5);
    }

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

/**
 * @brief Главное меню приложения.
 */
class MainMenuWidget : public QWidget {
    Q_OBJECT
public:
    QLabel*      lblTitle;
    QLabel*      lblSub;
    QLabel*      lblSelectHeader;
    QLabel*      lblSingleHeader;
    QLabel*      lblOpponents;
    QLabel*      lblMultiHeader;
    QFrame*      selectFrame;
    QFrame*      botFrame;
    QFrame*      netFrame;
    QComboBox*   comboGameType;
    QComboBox*   comboBots;
    QPushButton* btnStartBotGame;
    QPushButton* btnHostServer;
    QPushButton* btnConnectIP;
    QPushButton* btnSettings;
    QPushButton* btnRules;
    QLineEdit*   ipInput;

    explicit MainMenuWidget(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
};

/**
 * @brief Виджет стола для Техасского Холдема.
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

/**
 * @brief Виджет стола для Подкидного Дурака.
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

/**
 * @brief Виджет стола для игры Козёл.
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

/**
 * @brief Виджет игрового стола для Уно.
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

    void startSingleGame(int botCount);
    void handleAiLogic();
    void updateUI();
    void processNetAction(int senderId, const QJsonObject& json);
    void broadcastNetState();

    static void drawUnoCard(QPainter& p, const QRect& rect, const UnoCard* card, bool faceUp, bool selected = false);

    qreal   arrowAnimAngle = 0.0;
    QTimer* arrowAnimTimer = nullptr;
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

/**
 * @brief Главное окно приложения, координирующее переключение экранов.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    QStackedWidget* stackedWidget;
    MainMenuWidget* menuWidget;
    PokerWidget*    pokerWidget;
    DurakWidget*    durakWidget;
    KozelWidget*    kozelWidget;
    UnoWidget*      unoWidget;
    NetworkManager* netManager;

    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
};
