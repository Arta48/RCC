#include "MainMenuWidget.h"
#include "AppSettings.h"
#include "Audio.h"
#include "dialogs/SettingsDialog.h"

#include <QPainter>
#include <QScrollArea>
#include <QScroller>
#include <QScrollerProperties>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGuiApplication>
#include <QInputMethod>

#if defined(Q_OS_ANDROID)
#include <QJniObject>
inline void showAndroidKeyboardMainMenu() {
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
        auto activity = QJniObject(QNativeInterface::QAndroidApplication::context());
        if (!activity.isValid()) return;
        QJniObject imm = activity.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", QJniObject::fromString("input_method").object<jstring>());
        if (imm.isValid()) {
            QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
            if (window.isValid()) {
                QJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
                if (decorView.isValid()) imm.callMethod<jboolean>("showSoftInput", "(Landroid/view/View;I)Z", decorView.object<jobject>(), 2);
            }
        }
    });
}
#endif

MainMenuWidget::MainMenuWidget(QWidget* parent) : QWidget(parent) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    #if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QScroller::grabGesture(scrollArea->viewport(), QScroller::TouchGesture);
    auto* scroller = QScroller::scroller(scrollArea->viewport());
    QScrollerProperties sp = scroller->scrollerProperties();
    sp.setScrollMetric(QScrollerProperties::DragStartDistance, 0.008);
    sp.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0.0);
    scroller->setScrollerProperties(sp);
    #else
    QScroller::grabGesture(scrollArea->viewport(), QScroller::LeftMouseButtonGesture);
    #endif

    scrollArea->setStyleSheet(R"(
        QScrollArea { background: transparent; border: none; }
        QScrollBar:vertical { background: transparent; width: 5px; margin: 4px 2px 4px 0px; }
        QScrollBar::handle:vertical { background: rgba(251, 191, 36, 0.35); border-radius: 2px; min-height: 30px; }
        QScrollBar::handle:vertical:hover { background: rgba(251, 191, 36, 0.8); }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical, QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; height: 0px; }
    )");

    auto* container = new QWidget(scrollArea);
    container->setStyleSheet("background: transparent;");
    auto* mainLayout = new QVBoxLayout(container);
    mainLayout->setAlignment(Qt::AlignCenter);

    const QString frameStyle = "QFrame#panel { background: rgba(20, 27, 44, 0.88); border-radius: 14px; border: 1px solid rgba(251, 191, 36, 0.25); }";
    const QString comboStyle = "QComboBox { padding: 4px 10px; border-radius: 8px; background: #182234; color: #F9FAFB; border: 1px solid #374151; } "
    "QComboBox:hover { border: 1px solid #F59E0B; } "
    "QComboBox::drop-down { border: none; width: 28px; }";
    const QString inputStyle = "QLineEdit { padding: 4px 10px; border-radius: 8px; background: #182234; color: #F9FAFB; border: 1px solid #374151; } "
    "QLineEdit:focus { border: 1px solid #F59E0B; }";

    lblTitle = new QLabel(getLocalizedText("ROYAL CARD CLUB", "ROYAL CARD CLUB"), container);
    lblTitle->setStyleSheet("color: #FBBF24; font-weight: 900;");
    lblTitle->setAlignment(Qt::AlignCenter);

    lblSub = new QLabel(getLocalizedText("♠   ♥   ПОКЕР • ДУРАК • КОЗЁЛ • УНО   ♣   ♦", "♠   ♥   POKER • DURAK • KOZEL • UNO   ♣   ♦"), container);
    lblSub->setStyleSheet("color: #D97706; font-weight: 800;");
    lblSub->setAlignment(Qt::AlignCenter);

    selectFrame = new QFrame(container);
    selectFrame->setObjectName("panel");
    selectFrame->setStyleSheet(frameStyle);
    auto* selectLayout = new QVBoxLayout(selectFrame);

    lblSelectHeader = new QLabel(getLocalizedText("Выберите игру:", "Select game:"), selectFrame);
    lblSelectHeader->setStyleSheet("color: #F3F4F6; font-weight: bold;");

    comboGameType = new TouchComboBox(selectFrame);
    comboGameType->addItem(getLocalizedText("Покер (Texas Hold'em)", "Poker (Texas Hold'em)"), 0);
    comboGameType->addItem(getLocalizedText("Дурак (Подкидной)", "Durak (Podkidnoy)"), 1);
    comboGameType->addItem(getLocalizedText("Козёл", "Kozel"), 2);
    comboGameType->addItem(getLocalizedText("Уно", "UNO"), 3);
    comboGameType->setStyleSheet(comboStyle);
    comboGameType->setCursor(Qt::PointingHandCursor);

    selectLayout->addWidget(lblSelectHeader);
    selectLayout->addWidget(comboGameType);

    botFrame = new QFrame(container);
    botFrame->setObjectName("panel");
    botFrame->setStyleSheet(frameStyle);
    auto* botLayout = new QVBoxLayout(botFrame);

    lblSingleHeader = new QLabel(getLocalizedText("Одиночная игра (с Ботами):", "Singleplayer (vs Bots):"), botFrame);
    lblSingleHeader->setStyleSheet("color: #F3F4F6; font-weight: bold;");

    auto* comboLayout = new QHBoxLayout();
    lblOpponents = new QLabel(getLocalizedText("Соперники:", "Opponents:"), botFrame);
    lblOpponents->setStyleSheet("color: #9CA3AF; font-weight: bold;");

    comboBots = new TouchComboBox(botFrame);
    comboBots->addItem(getLocalizedText("1 Бот (Голова к голове)", "1 Bot (Heads Up)"), 1);
    comboBots->addItem(getLocalizedText("2 Бота (3 Макс.)", "2 Bots (3 Max)"), 2);
    comboBots->addItem(getLocalizedText("3 Бота (4 Макс.)", "3 Bots (4 Max)"), 3);
    comboBots->setStyleSheet(comboStyle);
    comboBots->setCursor(Qt::PointingHandCursor);

    comboLayout->addWidget(lblOpponents);
    comboLayout->addWidget(comboBots);

    btnStartBotGame = new QPushButton(getLocalizedText("Начать игру", "Start Game"), botFrame);
    btnStartBotGame->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #059669, stop:1 #10B981); color: white; font-weight: bold; border-radius: 8px; border: none; } "
    "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #10B981, stop:1 #34D399); }");
    btnStartBotGame->setCursor(Qt::PointingHandCursor);

    botLayout->addWidget(lblSingleHeader);
    botLayout->addLayout(comboLayout);
    botLayout->addWidget(btnStartBotGame);

    netFrame = new QFrame(container);
    netFrame->setObjectName("panel");
    netFrame->setStyleSheet(frameStyle);
    auto* netLayout = new QVBoxLayout(netFrame);

    lblMultiHeader = new QLabel(getLocalizedText("Сетевая игра (LAN / IP):", "Multiplayer (LAN / IP):"), netFrame);
    lblMultiHeader->setStyleSheet("color: #F3F4F6; font-weight: bold;");

    btnHostServer = new QPushButton(getLocalizedText("Создать сервер", "Create Server"), netFrame);
    btnHostServer->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1D4ED8, stop:1 #3B82F6); color: white; font-weight: bold; border-radius: 8px; border: none; } "
    "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2563EB, stop:1 #60A5FA); }");
    btnHostServer->setCursor(Qt::PointingHandCursor);

    auto* connectLayout = new QHBoxLayout();
    ipInput = new QLineEdit(netFrame);
    ipInput->setPlaceholderText("127.0.0.1");
    ipInput->setText("127.0.0.1");
    ipInput->setStyleSheet(inputStyle);
    ipInput->setFocusPolicy(Qt::StrongFocus);
    ipInput->setInputMethodHints(Qt::ImhUrlCharactersOnly | Qt::ImhNoPredictiveText);
    ipInput->installEventFilter(this);
    connect(ipInput, &QLineEdit::returnPressed, ipInput, &QLineEdit::clearFocus);

    btnConnectIP = new QPushButton(getLocalizedText("Подключиться", "Connect"), netFrame);
    btnConnectIP->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #B45309, stop:1 #F59E0B); color: white; font-weight: bold; border-radius: 8px; border: none; } "
    "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #D97706, stop:1 #FBBF24); }");
    btnConnectIP->setCursor(Qt::PointingHandCursor);

    connectLayout->addWidget(ipInput, 1);
    connectLayout->addWidget(btnConnectIP, 1);

    netLayout->addWidget(lblMultiHeader);
    netLayout->addWidget(btnHostServer);
    netLayout->addLayout(connectLayout);

    btnSettings = new QPushButton(getLocalizedText("Настройки", "Settings"), container);
    btnSettings->setStyleSheet("QPushButton { background: rgba(20, 27, 44, 0.88); color: #F3F4F6; font-weight: bold; border-radius: 10px; border: 1px solid rgba(251, 191, 36, 0.3); } "
    "QPushButton:hover { background: rgba(30, 41, 65, 0.95); border: 1px solid #F59E0B; }");
    btnSettings->setCursor(Qt::PointingHandCursor);

    btnRules = new QPushButton(getLocalizedText("Правила игры", "Game Rules"), container);
    btnRules->setStyleSheet("QPushButton { background: rgba(20, 27, 44, 0.88); color: #F3F4F6; font-weight: bold; border-radius: 10px; border: 1px solid rgba(251, 191, 36, 0.3); } "
    "QPushButton:hover { background: rgba(30, 41, 65, 0.95); border: 1px solid #F59E0B; }");
    btnRules->setCursor(Qt::PointingHandCursor);

    connect(btnSettings, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playSound(SoundEffect::ButtonClick);
        SettingsDialog dialog(this);
        dialog.exec();
    });

    mainLayout->addWidget(lblTitle);
    mainLayout->addWidget(lblSub);
    mainLayout->addWidget(selectFrame, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(6);
    mainLayout->addWidget(botFrame, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(6);
    mainLayout->addWidget(netFrame, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(6);
    mainLayout->addWidget(btnSettings, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(6);
    mainLayout->addWidget(btnRules, 0, Qt::AlignHCenter);

    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea);
}

void MainMenuWidget::resizeEvent(QResizeEvent* ev) {
    QWidget::resizeEvent(ev);
    const qreal s = std::clamp(std::min(width() / 1100.0, height() / 720.0), 0.6, 1.4);

    lblTitle->setFont(QFont(font().family(), qMax(18, qRound(38 * s)), QFont::Black));
    lblSub->setFont(QFont(font().family(), qMax(8, qRound(11 * s)), QFont::Bold));

    const int panelW = qRound(480 * s);
    selectFrame->setFixedWidth(panelW);
    botFrame->setFixedWidth(panelW);
    netFrame->setFixedWidth(panelW);
    btnSettings->setFixedWidth(panelW);
    btnRules->setFixedWidth(panelW);

    const int itemH = qRound(38 * s);
    const QFont fHeader(font().family(), qMax(9, qRound(12 * s)), QFont::Bold);
    const QFont fNorm(font().family(), qMax(9, qRound(12 * s)), QFont::Bold);

    lblSelectHeader->setFont(fHeader);
    lblSingleHeader->setFont(fHeader);
    lblOpponents->setFont(fHeader);
    lblMultiHeader->setFont(fHeader);

    comboGameType->setFixedHeight(itemH);
    comboGameType->setFont(fNorm);
    comboBots->setFixedHeight(itemH);
    comboBots->setFont(fNorm);
    btnStartBotGame->setFixedHeight(itemH);
    btnStartBotGame->setFont(fNorm);
    btnHostServer->setFixedHeight(itemH);
    btnHostServer->setFont(fNorm);
    ipInput->setFixedHeight(itemH);
    ipInput->setFont(fNorm);
    btnConnectIP->setFixedHeight(itemH);
    btnConnectIP->setFont(fNorm);
    btnSettings->setFixedHeight(itemH);
    btnSettings->setFont(fNorm);
    btnRules->setFixedHeight(itemH);
    btnRules->setFont(fNorm);
}

void MainMenuWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    #if defined(Q_OS_ANDROID)
    p.fillRect(rect(), QColor(6, 9, 16));
    #else
    QRadialGradient bg(width() / 2.0, height() / 2.0, qMax(width(), height()) * 0.65);
    bg.setColorAt(0.0, QColor(28, 42, 68));
    bg.setColorAt(0.5, QColor(14, 21, 35));
    bg.setColorAt(1.0, QColor(6, 9, 16));
    p.fillRect(rect(), bg);
    #endif

    const int suitSize = qBound(120, qRound(height() * 0.22), 260);
    const int currentPanelW = selectFrame ? selectFrame->width() : qMin(500, int(width() * 0.8));
    const double leftX = (width() - currentPanelW) / 2.0 - suitSize * 0.55;
    const double rightX = (width() + currentPanelW) / 2.0 + suitSize * 0.55;

    auto drawWatermark = [&](double x, double y, const QString& suit, double angle) {
        p.save();
        p.translate(x, y);
        p.rotate(angle);
        p.setFont(QFont(font().family(), suitSize, QFont::Bold));
        p.setPen(QColor(251, 191, 36, 25));
        p.drawText(-suitSize / 2, suitSize / 2, suit);
        p.restore();
    };

    drawWatermark(leftX, height() * 0.30, "♠", -14);
    drawWatermark(rightX, height() * 0.26, "♥", 12);
    drawWatermark(leftX - 20, height() * 0.76, "♣", 16);
    drawWatermark(rightX + 20, height() * 0.72, "♦", -10);
}

bool MainMenuWidget::eventFilter(QObject* watched, QEvent* event) {
    #if defined(Q_OS_ANDROID)
    if (watched == ipInput && (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::TouchEnd)) {
        ipInput->setFocus();
        QGuiApplication::inputMethod()->show();
        showAndroidKeyboardMainMenu();
    }
    #endif
    return QWidget::eventFilter(watched, event);
}
