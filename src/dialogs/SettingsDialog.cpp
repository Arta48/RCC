#include "SettingsDialog.h"
#include "AppSettings.h"
#include "Audio.h"
#include "widgets/TouchComboBox.h"

#include <QGuiApplication>
#include <QInputMethod>
#include <QWindow>

#if defined(Q_OS_ANDROID)
#include <QJniObject>

inline void showAndroidKeyboard() {
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
        auto activity = QJniObject(QNativeInterface::QAndroidApplication::context());
        if (!activity.isValid()) return;
        QJniObject imm = activity.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", QJniObject::fromString("input_method").object<jstring>());
        if (imm.isValid()) {
            QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
            if (window.isValid()) {
                QJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
                if (decorView.isValid()) {
                    imm.callMethod<jboolean>("showSoftInput", "(Landroid/view/View;I)Z", decorView.object<jobject>(), 2 /* SHOW_FORCED */);
                }
            }
        }
    });
}
#endif

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent ? parent->window() : nullptr) {
    setWindowTitle(getLocalizedText("Настройки", "Settings"));

    #if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setModal(true);
    #endif

    QWidget* topWin = parent ? parent->window() : nullptr;
    const int winW = topWin ? topWin->width() : (parent ? parent->width() : 1280);
    const int winH = topWin ? topWin->height() : (parent ? parent->height() : 720);
    const qreal s = std::clamp(std::min(winW / 1280.0, winH / 720.0), 0.7, 1.4);
    const int maxW = qMin(qRound(winW * 0.92), qRound(500 * s));
    const int maxH = qMin(qRound(winH * 0.90), qRound(440 * s));
    setGeometry((winW - maxW) / 2, (winH - maxH) / 2, maxW, maxH);

    const int fTitle = qMax(16, qRound(20 * s));
    const int fBase  = qMax(12, qRound(14 * s));
    const int pad    = qMax(5, qRound(8 * s));

    #if defined(Q_OS_ANDROID)
    const QString dlgBg = "#0B1120";
    const QString paneBg = "#0F172A";
    #else
    const QString dlgBg = "rgba(11, 17, 32, 0.96)";
    const QString paneBg = "rgba(15, 23, 42, 0.85)";
    #endif

    setStyleSheet(QString(R"(
        QDialog { background-color: %9; border: 1px solid rgba(251, 191, 36, 0.35); border-radius: %1px; }
        QLabel { color: #E2E8F0; font-size: %2px; font-weight: bold; }
        QTabWidget::pane { border: 1px solid rgba(251, 191, 36, 0.2); border-radius: %1px; background: %10; }
        QTabBar::tab { background: transparent; color: #94A3B8; padding: %3px %4px; font-size: %2px; font-weight: bold; border-bottom: 2px solid transparent; }
        QTabBar::tab:hover { color: #F8FAFC; }
        QTabBar::tab:selected { color: #FBBF24; border-bottom: 2px solid #FBBF24; background: rgba(251, 191, 36, 0.08); border-top-left-radius: 6px; border-top-right-radius: 6px; }
        QComboBox, QLineEdit, QSpinBox { padding: %3px %4px; border-radius: 6px; background: #1E293B; color: #F8FAFC; border: 1px solid #334155; font-size: %2px; }
        QComboBox:hover, QLineEdit:focus, QSpinBox:focus { border: 1px solid #F59E0B; }
        QComboBox::drop-down { border: none; width: %5px; }
        QCheckBox { color: #E2E8F0; font-size: %2px; font-weight: bold; spacing: 8px; }
        QSlider::groove:horizontal { height: %6px; background: #0F172A; border-radius: 3px; }
        QSlider::sub-page:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #D97706, stop:1 #FBBF24); border-radius: 3px; }
        QSlider::handle:horizontal { background: #FBBF24; width: %7px; height: %7px; margin-top: -5px; margin-bottom: -5px; border-radius: %8px; border: 2px solid #78350F; }
    )").arg(qRound(12 * s)).arg(fBase).arg(pad).arg(qRound(12 * s)).arg(qRound(24 * s)).arg(qRound(6 * s)).arg(qRound(16 * s)).arg(qRound(8 * s)).arg(dlgBg).arg(paneBg));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(qRound(16 * s), qRound(16 * s), qRound(16 * s), qRound(16 * s));
    mainLayout->setSpacing(qRound(12 * s));

    auto* title = new QLabel(getLocalizedText("⚙ НАСТРОЙКИ", "⚙ SETTINGS"), this);
    title->setStyleSheet(QString("color: #FBBF24; font-size: %1px; font-weight: 900; letter-spacing: 2px;").arg(fTitle));
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    auto* tabs = new QTabWidget(this);
    connect(tabs, &QTabWidget::currentChanged, this, [this](int) {
        this->update();
        #if defined(Q_OS_ANDROID)
        if (topLevelWidget()) {
            topLevelWidget()->update();
            if (topLevelWidget()->windowHandle()) {
                topLevelWidget()->windowHandle()->requestUpdate();
            }
        }
        #endif
    });

    // =========================================================================
    // ВКЛАДКА 1: ЗВУК
    // =========================================================================
    auto* tabAudio = new QWidget(tabs);
    auto* aLayout = new QVBoxLayout(tabAudio);
    aLayout->setContentsMargins(qRound(18 * s), qRound(18 * s), qRound(18 * s), qRound(18 * s));
    aLayout->setSpacing(qRound(18 * s));

    auto* mBox = new QHBoxLayout();
    auto* lblMusic = new QLabel(getLocalizedText("🎵 Музыка:", "🎵 Music:"), tabAudio);
    auto* sMusic = new QSlider(Qt::Horizontal, tabAudio);
    sMusic->setRange(0, 100);
    sMusic->setSingleStep(5);
    sMusic->setPageStep(5);

    const int curMusicVal = (qRound(AudioManager::instance().getMusicVolume() * 100.0f) / 5) * 5;
    sMusic->setValue(curMusicVal);

    auto* lblMusicVal = new QLabel(QString("%1%").arg(curMusicVal), tabAudio);
    lblMusicVal->setFixedWidth(qRound(45 * s));
    lblMusicVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblMusicVal->setStyleSheet(QString("color: #FCD34D; font-size: %1px; font-weight: bold;").arg(fBase));

    connect(sMusic, &QSlider::valueChanged, this, [lblMusicVal, sMusic](int val) {
        const int snapped = (val / 5) * 5;
        if (snapped != val) { sMusic->setValue(snapped); return; }
        AudioManager::instance().setMusicVolume(snapped / 100.0f);
        lblMusicVal->setText(QString("%1%").arg(snapped));
    });

    mBox->addWidget(lblMusic);
    mBox->addWidget(sMusic);
    mBox->addWidget(lblMusicVal);
    aLayout->addLayout(mBox);

    auto* sfxBox = new QHBoxLayout();
    auto* lblSfx = new QLabel(getLocalizedText("🔔 Эффекты:", "🔔 Effects:"), tabAudio);
    auto* sSfx = new QSlider(Qt::Horizontal, tabAudio);
    sSfx->setRange(0, 100);
    sSfx->setSingleStep(5);
    sSfx->setPageStep(5);

    const int curSfxVal = (qRound(AudioManager::instance().getSfxVolume() * 100.0f) / 5) * 5;
    sSfx->setValue(curSfxVal);

    auto* lblSfxVal = new QLabel(QString("%1%").arg(curSfxVal), tabAudio);
    lblSfxVal->setFixedWidth(qRound(45 * s));
    lblSfxVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblSfxVal->setStyleSheet(QString("color: #FCD34D; font-size: %1px; font-weight: bold;").arg(fBase));

    connect(sSfx, &QSlider::valueChanged, this, [lblSfxVal, sSfx](int val) {
        const int snapped = (val / 5) * 5;
        if (snapped != val) { sSfx->setValue(snapped); return; }
        AudioManager::instance().setSfxVolume(snapped / 100.0f);
        lblSfxVal->setText(QString("%1%").arg(snapped));
    });

    sfxBox->addWidget(lblSfx);
    sfxBox->addWidget(sSfx);
    sfxBox->addWidget(lblSfxVal);
    aLayout->addLayout(sfxBox);

    aLayout->addStretch();
    tabs->addTab(tabAudio, getLocalizedText("Звук", "Audio"));

    // =========================================================================
    // ВКЛАДКА 2: ВИЗУАЛ
    // =========================================================================
    auto* tabVisual = new QWidget(tabs);
    auto* vLayout = new QVBoxLayout(tabVisual);
    vLayout->setContentsMargins(qRound(18 * s), qRound(16 * s), qRound(18 * s), qRound(16 * s));
    vLayout->setSpacing(qRound(10 * s));

    auto* nickBox = new QHBoxLayout();
    nickBox->addWidget(new QLabel(getLocalizedText("Имя игрока:", "Player name:"), tabVisual));
    nickInput = new QLineEdit(AppSettings::instance().getNickname(), tabVisual);
    nickInput->setContextMenuPolicy(Qt::NoContextMenu);
    nickInput->setInputMethodHints(Qt::ImhNoPredictiveText);
    nickInput->installEventFilter(this);
    nickBox->addWidget(nickInput);
    vLayout->addLayout(nickBox);

    auto* avBox = new QHBoxLayout();
    avBox->addWidget(new QLabel(getLocalizedText("Аватар:", "Avatar:"), tabVisual));
    auto* avCombo = new TouchComboBox(tabVisual);
    avCombo->addItem(getLocalizedText("👑 Корона", "👑 Crown"), static_cast<int>(AvatarIcon::Crown));
    avCombo->addItem(getLocalizedText("💀 Череп", "💀 Skull"), static_cast<int>(AvatarIcon::Skull));
    avCombo->addItem(getLocalizedText("♠ Масть Пики", "♠ Spade Suit"), static_cast<int>(AvatarIcon::SuitSpade));
    avCombo->addItem(getLocalizedText("🃏 Джокер", "🃏 Joker"), static_cast<int>(AvatarIcon::Joker));
    avCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().getAvatar()));
    avBox->addWidget(avCombo);
    vLayout->addLayout(avBox);

    auto* colorBox = new QHBoxLayout();
    colorBox->addWidget(new QLabel(getLocalizedText("Цвет сукна:", "Table felt:"), tabVisual));
    auto* colorCombo = new TouchComboBox(tabVisual);
    colorCombo->addItem(getLocalizedText("🟢 Зелёный (Классика)", "🟢 Green (Classic)"), static_cast<int>(TableColor::ClassicGreen));
    colorCombo->addItem(getLocalizedText("🔴 Бордовый", "🔴 Burgundy"), static_cast<int>(TableColor::BurgundyRed));
    colorCombo->addItem(getLocalizedText("🔵 Тёмно-синий", "🔵 Dark Blue"), static_cast<int>(TableColor::DarkBlue));
    colorCombo->addItem(getLocalizedText("🖤 Покерный чёрный", "🖤 Poker Black"), static_cast<int>(TableColor::PokerBlack));
    colorCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().getTableColor()));
    colorBox->addWidget(colorCombo);
    vLayout->addLayout(colorBox);

    auto* shirtBox = new QHBoxLayout();
    shirtBox->addWidget(new QLabel(getLocalizedText("Рубашка карт:", "Card shirt:"), tabVisual));
    auto* shirtCombo = new TouchComboBox(tabVisual);
    shirtCombo->addItem(getLocalizedText("🟦 Классическая синяя", "🟦 Classic Blue"), static_cast<int>(CardShirtStyle::ClassicBlue));
    shirtCombo->addItem(getLocalizedText("🟥 Красный бархат", "🟥 Red Velvet"), static_cast<int>(CardShirtStyle::RedVelvet));
    shirtCombo->addItem(getLocalizedText("🟨 Золотая Royal", "🟨 Gold Royal"), static_cast<int>(CardShirtStyle::GoldRoyal));
    shirtCombo->addItem(getLocalizedText("⬛ Тёмная с узором", "⬛ Dark Pattern"), static_cast<int>(CardShirtStyle::DarkPattern));
    shirtCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().getCardShirt()));
    shirtBox->addWidget(shirtCombo);
    vLayout->addLayout(shirtBox);

    auto* chkFull = new QCheckBox(getLocalizedText("Полноэкранный режим (F11)", "Fullscreen Mode (F11)"), tabVisual);
    chkFull->setChecked(AppSettings::instance().getFullScreen());
    #if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    chkFull->setChecked(true);
    chkFull->setEnabled(false);
    chkFull->setText(getLocalizedText("Полноэкранный режим (Всегда включен)", "Fullscreen Mode (Always On)"));
    #endif
    vLayout->addWidget(chkFull);

    vLayout->addStretch();
    tabs->addTab(tabVisual, getLocalizedText("Визуал", "Visuals"));

    // =========================================================================
    // ВКЛАДКА 3: ГЕЙМПЛЕЙ И СЕТЬ
    // =========================================================================
    auto* tabGame = new QWidget(tabs);
    auto* gLayout = new QVBoxLayout(tabGame);
    gLayout->setContentsMargins(qRound(18 * s), qRound(18 * s), qRound(18 * s), qRound(18 * s));
    gLayout->setSpacing(qRound(14 * s));

    auto* chkAutoNext = new QCheckBox(getLocalizedText("Покер: Авто-старт следующей раздачи", "Poker: Auto-start next hand"), tabGame);
    chkAutoNext->setChecked(AppSettings::instance().getAutoNextHand());
    gLayout->addWidget(chkAutoNext);

    auto* chkHint = new QCheckBox(getLocalizedText("Покер: Подсказка моей комбинации", "Poker: Show hand hint"), tabGame);
    chkHint->setChecked(AppSettings::instance().getShowPokerHandHint());
    gLayout->addWidget(chkHint);

    auto* chkUnoStacking = new QCheckBox(getLocalizedText("Уно: Перевод штрафов (+2, +4)", "UNO: Stacking penalty cards (+2, +4)"), tabGame);
    chkUnoStacking->setChecked(AppSettings::instance().getUnoStacking());
    gLayout->addWidget(chkUnoStacking);

    auto* unoDrawBox = new QHBoxLayout();
    unoDrawBox->addWidget(new QLabel(getLocalizedText("Уно: Режим добора:", "UNO: Draw mode:"), tabGame));
    auto* unoDrawCombo = new TouchComboBox(tabGame);
    unoDrawCombo->addItem(getLocalizedText("Взять 1 карту и пас", "Draw 1 card and pass"), static_cast<int>(UnoDrawMode::DrawOne));
    unoDrawCombo->addItem(getLocalizedText("Тянуть до подходящей", "Draw until matching card"), static_cast<int>(UnoDrawMode::DrawUntilMatch));
    unoDrawCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().getUnoDrawMode()));
    unoDrawBox->addWidget(unoDrawCombo);
    gLayout->addLayout(unoDrawBox);

    auto* portBox = new QHBoxLayout();
    portBox->addWidget(new QLabel(getLocalizedText("Порт сервера:", "Server port:"), tabGame));
    portSpin = new QSpinBox(tabGame);
    portSpin->setRange(1024, 65535);
    portSpin->setValue(AppSettings::instance().getServerPort());
    portLineEdit = portSpin->findChild<QLineEdit*>();
    if (portLineEdit) {
        portLineEdit->setInputMethodHints(Qt::ImhDigitsOnly);
        portLineEdit->installEventFilter(this);
        connect(portLineEdit, &QLineEdit::returnPressed, portLineEdit, &QLineEdit::clearFocus);
    }
    portBox->addWidget(portSpin);
    gLayout->addLayout(portBox);

    gLayout->addStretch();
    tabs->addTab(tabGame, getLocalizedText("Геймплей и Сеть", "Gameplay && Network"));

    mainLayout->addWidget(tabs);

    auto* btnSave = new QPushButton(getLocalizedText("Сохранить и применить", "Save and apply"), this);
    btnSave->setCursor(Qt::PointingHandCursor);
    btnSave->setStyleSheet(QString(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #059669, stop:1 #10B981);
            color: white; font-weight: bold; font-size: %1px; padding: %2px; border-radius: 8px; border: 1px solid rgba(52, 211, 153, 0.4);
        }
        QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #10B981, stop:1 #34D399); }
    )").arg(fBase + 2).arg(qRound(10 * s)));

    connect(btnSave, &QPushButton::clicked, this, [=]() {
        const QString finalNick = nickInput->text().trimmed().isEmpty() ? getLocalizedText("Игрок", "Player") : nickInput->text().trimmed();
        AppSettings::instance().setNickname(finalNick);
        AppSettings::instance().setAvatar(static_cast<AvatarIcon>(avCombo->currentData().toInt()));
        AppSettings::instance().setTableColor(static_cast<TableColor>(colorCombo->currentData().toInt()));
        AppSettings::instance().setCardShirt(static_cast<CardShirtStyle>(shirtCombo->currentData().toInt()));
        AppSettings::instance().setUnoDrawMode(static_cast<UnoDrawMode>(unoDrawCombo->currentData().toInt()));
        AppSettings::instance().setUnoStacking(chkUnoStacking->isChecked());
        AppSettings::instance().setFullScreen(chkFull->isChecked());
        AppSettings::instance().setAutoNextHand(chkAutoNext->isChecked());
        AppSettings::instance().setShowPokerHandHint(chkHint->isChecked());
        AppSettings::instance().setServerPort(static_cast<quint16>(portSpin->value()));

        AudioManager::instance().setMusicVolume(sMusic->value() / 100.0f);
        AudioManager::instance().setSfxVolume(sSfx->value() / 100.0f);

        #if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
        if (parentWidget() && parentWidget()->window()) {
            if (AppSettings::instance().getFullScreen()) {
                parentWidget()->window()->showFullScreen();
            } else {
                parentWidget()->window()->showNormal();
            }
        }
        #endif

        AppSettings::instance().save();
        accept();
    });
    mainLayout->addWidget(btnSave);
}

void SettingsDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    this->update();
    #if defined(Q_OS_ANDROID)
    if (topLevelWidget()) {
        topLevelWidget()->update();
        if (topLevelWidget()->windowHandle()) {
            topLevelWidget()->windowHandle()->requestUpdate();
        }
    }
    #endif
}

bool SettingsDialog::eventFilter(QObject* watched, QEvent* event) {
    #if defined(Q_OS_ANDROID)
    if (watched == nickInput && (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::TouchEnd)) {
        nickInput->setFocus(Qt::MouseFocusReason);
        QGuiApplication::inputMethod()->show();
        showAndroidKeyboard();
    } else if (portLineEdit && watched == portLineEdit && (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::TouchEnd)) {
        portLineEdit->setFocus(Qt::MouseFocusReason);
        QGuiApplication::inputMethod()->show();
        showAndroidKeyboard();
    }
    #endif
    return QDialog::eventFilter(watched, event);
}
