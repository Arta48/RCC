#include <QTabWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextBrowser>
#include <QScrollArea>
#include <QScroller>
#include <QGuiApplication>
#include <QInputMethod>
#include <QMenu>
#include <QScrollerProperties>
#include <QWindow>

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QCoreApplication>
#include <QOpenGLWidget>
#endif

#include "AppSettings.h"
#include "MainWindow.h"
#include "Audio.h"

#if defined(Q_OS_ANDROID)
inline void showAndroidKeyboard() {
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
        auto activity = QJniObject(QNativeInterface::QAndroidApplication::context());
        if (!activity.isValid()) return;
        QJniObject imm = activity.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", QJniObject::fromString("input_method").object<jstring>());
        if (imm.isValid()) {
            QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
            if (window.isValid()) {
                QJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
                if (decorView.isValid()) imm.callMethod<jboolean>("showSoftInput", "(Landroid/view/View;I)Z", decorView.object<jobject>(), 2 /* SHOW_FORCED */);
            }
        }
    });
}
#endif

class TouchComboBox : public QComboBox {
public:
    explicit TouchComboBox(QWidget* parent = nullptr) : QComboBox(parent) {
        setFocusPolicy(Qt::NoFocus);
    }

    ~TouchComboBox() override {
        if (qApp) qApp->removeEventFilter(this);
        if (m_popupOverlay) {
            m_popupOverlay->deleteLater();
            m_popupOverlay = nullptr;
        }
    }

    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            if (m_popupOverlay && m_popupOverlay->isVisible()) {
                hidePopup();
                return;
            }
            showPopup();
            return;
        }
        QComboBox::mousePressEvent(e);
    }

    void showPopup() override {
        QWidget* topWin = window();
        if (!topWin) return;

        // Расчёт адаптивного масштабирования под размер окна
        qreal s = std::clamp(std::min(topWin->width() / 1280.0, topWin->height() / 720.0), 0.6, 1.4);
        int padH = qMax(6, qRound(10 * s));
        int padV = qMax(2, qRound(4 * s));
        int radius = qMax(4, qRound(6 * s));
        int overlayRadius = qMax(6, qRound(8 * s));
        int margin = qMax(2, qRound(4 * s));
        int spacing = qMax(1, qRound(2 * s));
        int itemH = height(); // Точная высота строки по высоте кнопки

        QString accentCol = palette().highlight().color().name();

        if (!m_popupOverlay) {
            m_popupOverlay = new QFrame(topWin);
            m_popupOverlay->setObjectName("comboPopupOverlay");

            auto* layout = new QVBoxLayout(m_popupOverlay);
            layout->setContentsMargins(margin, margin, margin, margin);
            layout->setSpacing(spacing);

            for (int i = 0; i < count(); ++i) {
                auto* btn = new QPushButton(itemText(i), m_popupOverlay);
                btn->setCursor(Qt::PointingHandCursor);
                connect(btn, &QPushButton::clicked, this, [this, i]() {
                    setCurrentIndex(i);
                    emit activated(i);
                    emit currentIndexChanged(i);
                    hidePopup();
                });
                layout->addWidget(btn);
            }
        } else {
            if (auto* l = m_popupOverlay->layout()) {
                l->setContentsMargins(margin, margin, margin, margin);
                l->setSpacing(spacing);
            }
        }
        qApp->installEventFilter(this);

        // Применяем адаптивные отступы и системный цвет (шрифт передаётся через setFont)
        m_popupOverlay->setStyleSheet(QString(R"(
            QFrame#comboPopupOverlay { background-color: #182234; border: 1px solid rgba(251, 191, 36, 0.4); border-radius: %1px; }
            QPushButton { background-color: transparent; color: #F8FAFC; border: none; border-radius: %2px; padding: %3px %4px; font-weight: bold; text-align: left; }
            QPushButton:hover, QPushButton:pressed { background-color: %5; color: #FFFFFF; }
        )").arg(overlayRadius).arg(radius).arg(padV).arg(padH).arg(accentCol));

        QFont f = font(); // Точный масштабированный шрифт комбобокса
        auto btns = m_popupOverlay->findChildren<QPushButton*>();
        for (int i = 0; i < btns.size() && i < count(); ++i) {
            btns[i]->setText(itemText(i));
            btns[i]->setFont(f);
            btns[i]->setFixedHeight(itemH);
        }

        // Позиционируем в координатах главного окна ровно по ширине кнопки
        QPoint posInWin = mapTo(topWin, QPoint(0, height() + 3));
        int popupW = width();
        int popupH = m_popupOverlay->sizeHint().height();

        // Если снизу выходит за пределы экрана — открываем над кнопкой
        if (posInWin.y() + popupH > topWin->height() - 10) {
            posInWin.setY(mapTo(topWin, QPoint(0, -popupH - 3)).y());
        }

        m_popupOverlay->setGeometry(posInWin.x(), posInWin.y(), popupW, popupH);
        m_popupOverlay->raise();
        m_popupOverlay->show();

        #if defined(Q_OS_ANDROID)
        if (topWin->windowHandle()) {
            topWin->windowHandle()->requestUpdate();
        }
        #endif
    }

    void hidePopup() override {
        if (m_popupOverlay && m_popupOverlay->isVisible()) {
            qApp->removeEventFilter(this);
            m_popupOverlay->hide();
            update();
            #if defined(Q_OS_ANDROID)
            if (window() && window()->windowHandle()) {
                window()->windowHandle()->requestUpdate();
            }
            #endif
        }
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (m_popupOverlay && m_popupOverlay->isVisible()) {
            if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) {
                QPoint globalPos;
                if (event->type() == QEvent::MouseButtonPress) {
                    globalPos = static_cast<QMouseEvent*>(event)->globalPosition().toPoint();
                } else {
                    auto* te = static_cast<QTouchEvent*>(event);
                    if (!te->points().isEmpty()) globalPos = te->points().first().globalPosition().toPoint();
                }
                QRect overlayRect(m_popupOverlay->mapToGlobal(QPoint(0, 0)), m_popupOverlay->size());
                QRect comboRect(mapToGlobal(QPoint(0, 0)), size());

                // Закрываем при клике мимо всплывающего окна
                if (!overlayRect.contains(globalPos)) {
                    hidePopup();
                    if (comboRect.contains(globalPos)) {
                        return true; // Поглощаем клик по самой кнопке, чтобы не открывалась заново
                    }
                }
            }
        }
        return QComboBox::eventFilter(watched, event);
    }

private:
    QFrame* m_popupOverlay = nullptr;
};

// ============================================================================
// НАСТРОЙКИ (ДИАЛОГОВОЕ ОКНО)
// ============================================================================

class SettingsDialog : public QDialog {
public:
    QLineEdit* nickInput    = nullptr;
    QLineEdit* portLineEdit = nullptr;
    QSpinBox*  portSpin     = nullptr;

    explicit SettingsDialog(QWidget* parent = nullptr) : QDialog(parent ? parent->window() : nullptr) {
        setWindowTitle(getLocalizedText("Настройки", "Settings"));
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false); // Убираем сдвиг тача в диалоге
        setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
        setModal(true);
#endif

        QWidget* topWin = parent ? parent->window() : nullptr;
        int winW = topWin ? topWin->width() : (parent ? parent->width() : 1280);
        int winH = topWin ? topWin->height() : (parent ? parent->height() : 720);
        qreal s = std::clamp(std::min(winW / 1280.0, winH / 720.0), 0.7, 1.4);
        int maxW = qMin(qRound(winW * 0.92), qRound(500 * s));
        int maxH = qMin(qRound(winH * 0.90), qRound(440 * s));
        setGeometry((winW - maxW) / 2, (winH - maxH) / 2, maxW, maxH);

        int fTitle = qMax(16, qRound(20 * s));
        int fBase  = qMax(12, qRound(14 * s));
        int pad    = qMax(5, qRound(8 * s));

        // На Android делаем непрозрачный фон для избежания артефактов GPU Mali
#if defined(Q_OS_ANDROID)
        QString dlgBg = "#0B1120";
        QString paneBg = "#0F172A";
#else
        QString dlgBg = "rgba(11, 17, 32, 0.96)";
        QString paneBg = "rgba(15, 23, 42, 0.85)";
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

        // ВКЛАДКА 1: ЗВУК
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

        int curMusicVal = (qRound(AudioManager::instance().getMusicVolume() * 100.0f) / 5) * 5;
        sMusic->setValue(curMusicVal);

        auto* lblMusicVal = new QLabel(QString("%1%").arg(curMusicVal), tabAudio);
        lblMusicVal->setFixedWidth(qRound(45 * s));
        lblMusicVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lblMusicVal->setStyleSheet(QString("color: #FCD34D; font-size: %1px; font-weight: bold;").arg(fBase));

        connect(sMusic, &QSlider::valueChanged, this, [lblMusicVal, sMusic](int val) {
            int snapped = (val / 5) * 5;
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

        int curSfxVal = (qRound(AudioManager::instance().getSfxVolume() * 100.0f) / 5) * 5;
        sSfx->setValue(curSfxVal);

        auto* lblSfxVal = new QLabel(QString("%1%").arg(curSfxVal), tabAudio);
        lblSfxVal->setFixedWidth(qRound(45 * s));
        lblSfxVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lblSfxVal->setStyleSheet(QString("color: #FCD34D; font-size: %1px; font-weight: bold;").arg(fBase));

        connect(sSfx, &QSlider::valueChanged, this, [lblSfxVal, sSfx](int val) {
            int snapped = (val / 5) * 5;
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

        // ВКЛАДКА 2: ВИЗУАЛ
        auto* tabVisual = new QWidget(tabs);
        auto* vLayout = new QVBoxLayout(tabVisual);
        vLayout->setContentsMargins(qRound(18 * s), qRound(16 * s), qRound(18 * s), qRound(16 * s));
        vLayout->setSpacing(qRound(10 * s));

        auto* nickBox = new QHBoxLayout();
        nickBox->addWidget(new QLabel(getLocalizedText("Имя игрока:", "Player name:"), tabVisual));
        nickInput = new QLineEdit(AppSettings::instance().nickname, tabVisual);
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
        avCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().avatar));

        avBox->addWidget(avCombo);
        vLayout->addLayout(avBox);

        auto* colorBox = new QHBoxLayout();
        colorBox->addWidget(new QLabel(getLocalizedText("Цвет сукна:", "Table felt:"), tabVisual));
        auto* colorCombo = new TouchComboBox(tabVisual);
        colorCombo->addItem(getLocalizedText("🟢 Зелёный (Классика)", "🟢 Green (Classic)"), static_cast<int>(TableColor::ClassicGreen));
        colorCombo->addItem(getLocalizedText("🔴 Бордовый", "🔴 Burgundy"), static_cast<int>(TableColor::BurgundyRed));
        colorCombo->addItem(getLocalizedText("🔵 Тёмно-синий", "🔵 Dark Blue"), static_cast<int>(TableColor::DarkBlue));
        colorCombo->addItem(getLocalizedText("🖤 Покерный чёрный", "🖤 Poker Black"), static_cast<int>(TableColor::PokerBlack));
        colorCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().tableColor));

        colorBox->addWidget(colorCombo);
        vLayout->addLayout(colorBox);

        auto* shirtBox = new QHBoxLayout();
        shirtBox->addWidget(new QLabel(getLocalizedText("Рубашка карт:", "Card shirt:"), tabVisual));
        auto* shirtCombo = new TouchComboBox(tabVisual);
        shirtCombo->addItem(getLocalizedText("🟦 Классическая синяя", "🟦 Classic Blue"), static_cast<int>(CardShirtStyle::ClassicBlue));
        shirtCombo->addItem(getLocalizedText("🟥 Красный бархат", "🟥 Red Velvet"), static_cast<int>(CardShirtStyle::RedVelvet));
        shirtCombo->addItem(getLocalizedText("🟨 Золотая Royal", "🟨 Gold Royal"), static_cast<int>(CardShirtStyle::GoldRoyal));
        shirtCombo->addItem(getLocalizedText("⬛ Тёмная с узором", "⬛ Dark Pattern"), static_cast<int>(CardShirtStyle::DarkPattern));
        shirtCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().cardShirt));
        shirtBox->addWidget(shirtCombo);
        vLayout->addLayout(shirtBox);

        auto* chkFull = new QCheckBox(getLocalizedText("Полноэкранный режим (F11)", "Fullscreen Mode (F11)"), tabVisual);
        chkFull->setChecked(AppSettings::instance().fullScreen);
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
        chkFull->setChecked(true);
        chkFull->setEnabled(false);
        chkFull->setText(getLocalizedText("Полноэкранный режим (Всегда включен)", "Fullscreen Mode (Always On)"));
#endif
        vLayout->addWidget(chkFull);


        vLayout->addStretch();
        tabs->addTab(tabVisual, getLocalizedText("Визуал", "Visuals"));

        // ВКЛАДКА 3: ГЕЙМПЛЕЙ И СЕТЬ
        auto* tabGame = new QWidget(tabs);
        auto* gLayout = new QVBoxLayout(tabGame);
        gLayout->setContentsMargins(qRound(18 * s), qRound(18 * s), qRound(18 * s), qRound(18 * s));
        gLayout->setSpacing(qRound(14 * s));

        auto* chkAutoNext = new QCheckBox(getLocalizedText("Покер: Авто-старт следующей раздачи", "Poker: Auto-start next hand"), tabGame);
        chkAutoNext->setChecked(AppSettings::instance().autoNextHand);
        gLayout->addWidget(chkAutoNext);

        auto* chkHint = new QCheckBox(getLocalizedText("Покер: Подсказка моей комбинации", "Poker: Show hand hint"), tabGame);
        chkHint->setChecked(AppSettings::instance().showPokerHandHint);
        gLayout->addWidget(chkHint);

        auto* chkUnoStacking = new QCheckBox(getLocalizedText("Уно: Перевод штрафов (+2, +4)", "UNO: Stacking penalty cards (+2, +4)"), tabGame);
        chkUnoStacking->setChecked(AppSettings::instance().unoStacking);
        gLayout->addWidget(chkUnoStacking);

        auto* unoDrawBox = new QHBoxLayout();
        unoDrawBox->addWidget(new QLabel(getLocalizedText("Уно: Режим добора:", "UNO: Draw mode:"), tabGame));
        auto* unoDrawCombo = new TouchComboBox(tabGame);
        unoDrawCombo->addItem(getLocalizedText("Взять 1 карту и пас", "Draw 1 card and pass"), static_cast<int>(UnoDrawMode::DrawOne));
        unoDrawCombo->addItem(getLocalizedText("Тянуть до подходящей", "Draw until matching card"), static_cast<int>(UnoDrawMode::DrawUntilMatch));
        unoDrawCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().unoDrawMode));
        unoDrawBox->addWidget(unoDrawCombo);
        gLayout->addLayout(unoDrawBox);

        auto* portBox = new QHBoxLayout();
        portBox->addWidget(new QLabel(getLocalizedText("Порт сервера:", "Server port:"), tabGame));
        portSpin = new QSpinBox(tabGame);
        portSpin->setRange(1024, 65535);
        portSpin->setValue(AppSettings::instance().serverPort);
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
            AppSettings::instance().nickname          = nickInput->text().trimmed().isEmpty() ? getLocalizedText("Игрок", "Player") : nickInput->text().trimmed();
            AppSettings::instance().avatar            = static_cast<AvatarIcon>(avCombo->currentData().toInt());
            AppSettings::instance().tableColor        = static_cast<TableColor>(colorCombo->currentData().toInt());
            AppSettings::instance().cardShirt         = static_cast<CardShirtStyle>(shirtCombo->currentData().toInt());
            AppSettings::instance().unoDrawMode       = static_cast<UnoDrawMode>(unoDrawCombo->currentData().toInt());
            AppSettings::instance().unoStacking       = chkUnoStacking->isChecked();
            AppSettings::instance().fullScreen        = chkFull->isChecked();
            AppSettings::instance().autoNextHand      = chkAutoNext->isChecked();
            AppSettings::instance().showPokerHandHint = chkHint->isChecked();
            AppSettings::instance().serverPort        = static_cast<quint16>(portSpin->value());

            AudioManager::instance().setMusicVolume(sMusic->value() / 100.0f);
            AudioManager::instance().setSfxVolume(sSfx->value() / 100.0f);

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
            if (parentWidget() && parentWidget()->window()) {
                if (AppSettings::instance().fullScreen) parentWidget()->window()->showFullScreen();
                else parentWidget()->window()->showNormal();
            }
#endif

            AppSettings::instance().save();
            accept();
        });
        mainLayout->addWidget(btnSave);
    }

protected:
    void showEvent(QShowEvent* event) override {
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

    bool eventFilter(QObject* watched, QEvent* event) override {
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
};

// ============================================================================
// ПРАВИЛА (ДИАЛОГОВОЕ ОКНО)
// ============================================================================

class RulesDialog : public QDialog {
public:
    explicit RulesDialog(int defaultTabIndex = 0, QWidget* parent = nullptr) : QDialog(parent ? parent->window() : nullptr) {
        setWindowTitle(getLocalizedText("Правила игры", "Game Rules"));
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false); // Убираем сдвиг тача в диалоге
        setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
        setModal(true);
#endif

        QWidget* topWin = parent ? parent->window() : nullptr;
        int winW = topWin ? topWin->width() : (parent ? parent->width() : 1280);
        int winH = topWin ? topWin->height() : (parent ? parent->height() : 720);
        qreal s = std::clamp(std::min(winW / 1280.0, winH / 720.0), 0.7, 1.4);
        int maxW = qMin(qRound(winW * 0.92), qRound(660 * s));
        int maxH = qMin(qRound(winH * 0.90), qRound(480 * s));
        setGeometry((winW - maxW) / 2, (winH - maxH) / 2, maxW, maxH);

        int fTitle = qMax(16, qRound(20 * s));
        int fBase  = qMax(12, qRound(13 * s));
        int pad    = qMax(5, qRound(8 * s));

#if defined(Q_OS_ANDROID)
        QString dlgBg = "#0B1120";
        QString paneBg = "#0F172A";
        QString textBg = "#0F172A";
#else
        QString dlgBg = "rgba(11, 17, 32, 0.96)";
        QString paneBg = "rgba(15, 23, 42, 0.90)";
        QString textBg = "transparent";
#endif

        setStyleSheet(QString(R"(
            QDialog { background-color: %6; border: 1px solid rgba(251, 191, 36, 0.35); border-radius: %1px; }
            QLabel { color: #FBBF24; font-size: %2px; font-weight: 900; }
            QTabWidget::pane { border: 1px solid rgba(251, 191, 36, 0.2); border-radius: %1px; background: %7; }
            QTabBar::tab { background: transparent; color: #94A3B8; padding: %3px %4px; font-size: %5px; font-weight: bold; border-bottom: 2px solid transparent; }
            QTabBar::tab:hover { color: #F8FAFC; }
            QTabBar::tab:selected { color: #FBBF24; border-bottom: 2px solid #FBBF24; background: rgba(251, 191, 36, 0.08); border-top-left-radius: 6px; border-top-right-radius: 6px; }
            QTextBrowser { background: %8; border: none; color: #E2E8F0; font-size: %5px; line-height: 1.5; padding: %3px; }
            QScrollBar:vertical { background: #0F172A; width: 8px; border-radius: 4px; }
            QScrollBar::handle:vertical { background: #334155; border-radius: 4px; }
        )").arg(qRound(12 * s)).arg(fTitle).arg(pad).arg(qRound(14 * s)).arg(fBase).arg(dlgBg).arg(paneBg).arg(textBg));

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(qRound(16 * s), qRound(16 * s), qRound(16 * s), qRound(16 * s));
        mainLayout->setSpacing(qRound(10 * s));

        auto* title = new QLabel(getLocalizedText("📖 ПРАВИЛА ИГР", "📖 GAME RULES"), this);
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

        auto createRuleTab = [&](const QString& textHtml) -> QWidget* {
            auto* page = new QWidget(tabs);
            auto* l = new QVBoxLayout(page);
            l->setContentsMargins(6, 6, 6, 6);
            auto* browser = new QTextBrowser(page);
            browser->setTextInteractionFlags(Qt::NoTextInteraction);
            browser->setHtml(textHtml);
            l->addWidget(browser);
            return page;
        };

        QString pokerRules = getLocalizedText(
            "<h3>Покер (Texas Hold'em)</h3>"
            "<p><b>Колода:</b> 52 карты. Каждый игрок получает по 2 закрытые карты, а на стол поочерёдно выкладываются 5 общих карт (Флоп, Тёрн, Ривер).</p>"
            "<p><b>Цель:</b> Собрать сильнейшую 5-карточную комбинацию (из 7 доступных карт) или вынудить всех соперников сбросить карты (Fold).</p>"
            "<p><b>Иерархия комбинаций:</b> Старшая карта → Пара → Две пары → Тройка (Сет / Трипс) → Стрит → Флеш → Фулл-Хаус → Каре → Стрит-Флеш → Роял-Флеш.</p>"
            "<p><b>Действия:</b> Check (Чек), Call (Уравнять), Raise (Повысить), Fold (Сброс), All-In (Ва-банк).</p>",
            "<h3>Poker (Texas Hold'em)</h3>"
            "<p><b>Deck:</b> 52 cards. Each player receives 2 hole cards, followed by 5 community cards dealt on board (Flop, Turn, River).</p>"
            "<p><b>Goal:</b> Make the best 5-card combination (using 7 available cards) or force all opponents to fold.</p>"
            "<p><b>Hand Rankings:</b> High Card → Pair → Two Pair → Three of a Kind → Straight → Flush → Full House → Four of a Kind → Straight Flush → Royal Flush.</p>"
            "<p><b>Actions:</b> Check, Call, Raise, Fold, All-In.</p>"
        );

        QString durakRules = getLocalizedText(
            "<h3>Дурак (Подкидной)</h3>"
            "<p><b>Колода:</b> 36 карт (от 6 до Туза). Игрокам раздаётся по 6 карт, нижняя карта колоды открывается и определяет козырь.</p>"
            "<p><b>Цель:</b> Первым сбросить все карты.</p>"
            "<p><b>Ход игры:</b> Игрок с младшим козырем ходит первым. Атаковать можно картами одного достоинства. Отбивающийся бьёт карты старшей картой той же масти или козырем.</p>"
            "<p><b>Подкидывание:</b> Атакующий и другие игроки могут подкидывать карты совпадающих достоинств (не более количества карт у отбивающегося и не более 6).</p>"
            "<p><b>Добор:</b> После отбоя (или взятия) игроки по очереди добирают до 6 карт: сначала атакующий, затем подкидывавшие, последним — защитник.</p>",
            "<h3>Durak (Podkidnoy)</h3>"
            "<p><b>Deck:</b> 36 cards (6 to Ace). Players receive 6 cards each. The bottom card is revealed to set the trump suit.</p>"
            "<p><b>Goal:</b> Shed all cards first.</p>"
            "<p><b>Gameplay:</b> Lowest trump leads first. Attack with matching ranks. Defender beats attacks with a higher card of the same suit or trump.</p>"
            "<p><b>Tossing:</b> Players can toss additional cards matching any rank on the table (up to defender's hand size, max 6 per bout).</p>"
            "<p><b>Drawing:</b> After a bout (Done / Taken), players replenish back up to 6 cards in clockwise order: attacker first, tossers next, defender last.</p>"
        );

        QString kozelRules = getLocalizedText(
            "<h3>Козёл</h3>"
            "<p><b>Колода:</b> 36 карт. Раздаётся по 4 карты, козырь определяется случайным срезом колоды и убирается обратно.</p>"
            "<p><b>Особое старшинство:</b> Десятка старше Короля! (6, 7, 8, 9, Валет, Дама, Король, 10, Туз).</p>"
            "<p><b>Заход:</b> Разрешается заходить одной или несколькими картами одной масти. Соперники обязаны перекрыть каждую карту или сбросить столько же карт втемную.</p>"
            "<p><b>Очки:</b> Туз = 11, 10 = 10, Король = 4, Дама = 3, Валет = 2 (всего 120 очков). Для победы в раздаче нужно набрать более 60 очков.</p>",
            "<h3>Kozel</h3>"
            "<p><b>Deck:</b> 36 cards. 4 cards dealt per player. Trump is revealed by cutting the deck and then hidden.</p>"
            "<p><b>Card Ranking:</b> 10 is higher than King! (6, 7, 8, 9, Jack, Queen, King, 10, Ace).</p>"
            "<p><b>Tricks:</b> Players lead with 1 or more cards of the same suit. Opponents must beat every card (higher rank or trump) or sluff the same number face down.</p>"
            "<p><b>Points:</b> Ace = 11, 10 = 10, King = 4, Queen = 3, Jack = 2 (Total: 120 pts). Collect > 60 points to win.</p>"
        );

        QString unoRules = getLocalizedText(
            "<h3>Уно</h3>"
            "<p><b>Колода:</b> 108 карт. Раздаётся по 7 карт. Игроки ходят по очереди в стопку сброса, сопоставляя карту по цвету, цифре или символу.</p>"
            "<p><b>Спец-карты:</b> Пропуск хода (Skip), Смена направления (Reverse), Возьми две (+2), Дикая карта (Wild) и Дикая +4.</p>"
            "<p><b>Цель:</b> Первым избавиться от всех карт на руках.</p>"
            "<p><b>Правило «УНО!»:</b> При ходе с 2 карт до 1 игрок обязан объявить «Уно!». Если он забыл это сделать, соперники могут нажать «Поймать» и выдать ему штраф +2 карты.</p>"
            "<p><b>Добор:</b> Если нет карт для хода, игрок берёт карту из колоды (её можно сразу сыграть, если она подходит).</p>",
            "<h3>UNO</h3>"
            "<p><b>Deck:</b> 108 cards. 7 cards dealt. Match the discard pile by Color, Number, or Action symbol.</p>"
            "<p><b>Action Cards:</b> Skip, Reverse, Draw Two (+2), Wild, Wild Draw Four (+4).</p>"
            "<p><b>Goal:</b> Be the first player to get rid of all your cards.</p>"
            "<p><b>'UNO!' Rule:</b> When playing down to 1 card, declare 'UNO!'. If you forget, opponents can press 'Catch' to give you a 2-card penalty.</p>"
            "<p><b>Draw:</b> If you have no matching card, draw from the deck (can be played immediately if valid).</p>"
        );

        tabs->addTab(createRuleTab(pokerRules), getLocalizedText("Покер", "Poker"));
        tabs->addTab(createRuleTab(durakRules), getLocalizedText("Дурак", "Durak"));
        tabs->addTab(createRuleTab(kozelRules), getLocalizedText("Козёл", "Kozel"));
        tabs->addTab(createRuleTab(unoRules),   getLocalizedText("Уно", "UNO"));

        tabs->setCurrentIndex(qBound(0, defaultTabIndex, 3));
        mainLayout->addWidget(tabs);

        auto* btnClose = new QPushButton(getLocalizedText("Закрыть", "Close"), this);
        btnClose->setCursor(Qt::PointingHandCursor);
        btnClose->setStyleSheet(QString(R"(
            QPushButton {
                background: rgba(30, 41, 59, 0.9); color: #F8FAFC; font-weight: bold; font-size: %1px; padding: %2px; border-radius: 8px; border: 1px solid #475569;
            }
            QPushButton:hover { background: #334155; border: 1px solid #FBBF24; }
        )").arg(fBase + 1).arg(qRound(8 * s)));
        connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
        mainLayout->addWidget(btnClose);
    }

protected:
    void showEvent(QShowEvent* event) override {
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

    void hideEvent(QHideEvent* event) override {
        QDialog::hideEvent(event);
#if defined(Q_OS_ANDROID)
        if (topLevelWidget()) {
            topLevelWidget()->update();
            if (topLevelWidget()->windowHandle()) {
                topLevelWidget()->windowHandle()->requestUpdate();
            }
        }
#endif
    }
};


// ============================================================================
// BASE TABLE WIDGET
// ============================================================================

BaseTableWidget::BaseTableWidget(QWidget* parent) : QWidget(parent) {
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    setMinimumSize(640, 360);
#endif
    setMouseTracking(true);

    btnBackMenu = new QPushButton(getLocalizedText("← В Меню", "← Menu"), this);
    lblStatus   = new QLabel("", this);
    btnNextHand = new QPushButton(getLocalizedText("ИГРАТЬ ЗАНОВО", "PLAY AGAIN"), this);
    btnSettings = new QPushButton("⚙", this);
    btnRules    = new QPushButton("📖", this);

    btnBackMenu->setStyleSheet("QPushButton { background: rgba(0,0,0,0.5); color: #D1D5DB; border-radius: 6px; padding: 5px; font-weight: bold; } QPushButton:hover { background: rgba(0,0,0,0.7); color: white; }");
    lblStatus->setStyleSheet("QLabel { color: #F3F4F6; font-weight: bold; }");
    btnNextHand->setStyleSheet("QPushButton { background: #10B981; color: white; font-weight: bold; border-radius: 8px; padding: 10px; } QPushButton:hover { background: #34D399; }");
    btnSettings->setStyleSheet("QPushButton { background: rgba(0,0,0,0.5); color: #D1D5DB; border-radius: 6px; font-weight: bold; } QPushButton:hover { background: rgba(0,0,0,0.7); color: #FBBF24; }");
    btnRules->setStyleSheet("QPushButton { background: rgba(0,0,0,0.5); color: #D1D5DB; border-radius: 6px; font-weight: bold; } QPushButton:hover { background: rgba(0,0,0,0.7); color: #FBBF24; }");

    btnBackMenu->setCursor(Qt::PointingHandCursor);
    btnNextHand->setCursor(Qt::PointingHandCursor);
    btnSettings->setCursor(Qt::PointingHandCursor);
    btnRules->setCursor(Qt::PointingHandCursor);
    btnNextHand->hide();

    connect(btnBackMenu, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playSound(SoundEffect::ButtonClick);
        if (onBackToMenuCallback) onBackToMenuCallback();
    });

    connect(btnSettings, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playSound(SoundEffect::ButtonClick);
        if (onOpenSettingsCallback) onOpenSettingsCallback();
    });

    connect(btnRules, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playSound(SoundEffect::ButtonClick);
        if (onOpenRulesCallback) onOpenRulesCallback();
    });
}

void BaseTableWidget::resizeEvent(QResizeEvent* ev) {
    QWidget::resizeEvent(ev);
    qreal s = getScale();

    int btnH = qRound(36 * s);
    int btnBackW = qRound(110 * s);
    int iconBtnW = qRound(40 * s);
    int leftOffset = getSafeLeftMargin();

    btnBackMenu->setGeometry(leftOffset, qRound(15 * s), btnBackW, btnH);

    btnSettings->setGeometry(width() - iconBtnW - qRound(15 * s), qRound(15 * s), iconBtnW, btnH);
    btnSettings->setFont(QFont(font().family(), qMax(12, qRound(20 * s)), QFont::Bold));

    btnRules->setGeometry(width() - iconBtnW * 2 - qRound(25 * s), qRound(15 * s), iconBtnW, btnH);
    btnRules->setFont(QFont(font().family(), qMax(12, qRound(20 * s)), QFont::Bold));

    lblStatus->setGeometry(leftOffset + btnBackW + qRound(15 * s), qRound(15 * s), width() - leftOffset - btnBackW - iconBtnW * 2 - qRound(75 * s), btnH);
    lblStatus->setFont(QFont(font().family(), qMax(9, qRound(15 * s)), QFont::Bold));

    int nextW = qRound(260 * s);
    int nextH = qRound(55 * s);
    btnNextHand->setGeometry(width() / 2 - nextW / 2, height() / 2 + qRound(60 * s), nextW, nextH);
    btnNextHand->setFont(QFont(font().family(), qMax(9, qRound(14 * s)), QFont::Bold));
}

void BaseTableWidget::drawTableFelt(QPainter& p) {
    p.fillRect(rect(), QColor(20, 20, 20));
    qreal s = getScale();
    int margin = qMax(6, qRound(16 * s));
    QRect feltRect = rect().adjusted(margin, margin, -margin, -margin);

    // Радиус закругления всегда рассчитывается от реальных пропорций экрана
    int cornerRadius = qMin(feltRect.width() / 4, feltRect.height() / 2);
    QPainterPath tablePath;
    tablePath.addRoundedRect(feltRect, cornerRadius, cornerRadius);

    QColor c1, c2;
    switch (AppSettings::instance().tableColor) {
        case TableColor::BurgundyRed: c1 = QColor(140, 20, 40); c2 = QColor(60, 5, 15); break;
        case TableColor::DarkBlue:   c1 = QColor(25, 60, 120); c2 = QColor(10, 25, 55); break;
        case TableColor::PokerBlack: c1 = QColor(45, 45, 50); c2 = QColor(15, 15, 20); break;
        default:                     c1 = QColor(30, 130, 60); c2 = QColor(10, 60, 20); break;
    }

    QRadialGradient bgGrad(width() / 2, height() / 2, qMax(width(), height()));
    bgGrad.setColorAt(0, c1);
    bgGrad.setColorAt(1, c2);
    p.fillPath(tablePath, bgGrad);

    p.setPen(QPen(QColor(0, 0, 0, 120), qMax(2, qRound(4 * s))));
    p.drawPath(tablePath);
}

void BaseTableWidget::drawGameOverBanner(QPainter& p, const QString& message) {
    qreal s = getScale();
#if defined(Q_OS_ANDROID)
    p.fillRect(rect(), QColor(0, 0, 0, 180));
    p.setBrush(QColor(15, 23, 42));
#else
    p.fillRect(rect(), QColor(0, 0, 0, 110));
    p.setBrush(QColor(15, 23, 42, 240));
#endif

    int bannerW = qRound(440 * s);
    int bannerH = qRound(65 * s);
    QRect bannerRect(width() / 2 - bannerW / 2, height() / 2 - qRound(125 * s), bannerW, bannerH);

    p.setBrush(QColor(15, 23, 42, 240));
    p.setPen(QPen(QColor(251, 191, 36, 220), qMax(1, qRound(2 * s))));
    p.drawRoundedRect(bannerRect, qRound(10 * s), qRound(10 * s));

    p.setPen(QColor(252, 211, 77));
    p.setFont(QFont(font().family(), qMax(10, qRound(16 * s)), QFont::Bold));
    p.drawText(bannerRect, Qt::AlignCenter | Qt::TextWordWrap, message);
}

QVector<QPoint> BaseTableWidget::getSeatPositions(int numPlayers, int width, int height, int bottomYOffset, int topYOffset) {
    qreal s = std::clamp(std::min(width / 1280.0, height / 720.0), 0.45, 2.5);
    int sideX = qMax(80, qRound(120 * s));
    int sideY = height / 2 - qRound(70 * s);

    if (numPlayers == 2) {
        return {
            QPoint(width / 2, height - bottomYOffset),
            QPoint(width / 2, topYOffset)
        };
    }
    if (numPlayers == 3) {
        return {
            QPoint(width / 2, height - bottomYOffset),
            QPoint(sideX, sideY),
            QPoint(width - sideX, sideY)
        };
    }
    return {
        QPoint(width / 2, height - bottomYOffset),
        QPoint(sideX, sideY),
        QPoint(width / 2, topYOffset),
        QPoint(width - sideX, sideY)
    };
}

void BaseTableWidget::drawCard(QPainter& p, const QRect& rect, const Card* card, bool faceUp, bool selected) {
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 120));
    p.drawRoundedRect(rect.translated(4, 5), 6, 6);

    QPainterPath path;
    path.addRoundedRect(rect, 6, 6);
    p.fillPath(path, selected ? QColor(254, 240, 138) : Qt::white);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(150, 150, 150), 1));
    p.drawPath(path);

    if (!faceUp || !card) {
        QRect inner = rect.adjusted(5, 5, -5, -5);
        p.setClipRect(inner);

        QColor shirtColor;
        switch (AppSettings::instance().cardShirt) {
            case CardShirtStyle::RedVelvet:   shirtColor = QColor(185, 28, 28); break;
            case CardShirtStyle::GoldRoyal:   shirtColor = QColor(180, 140, 20); break;
            case CardShirtStyle::DarkPattern: shirtColor = QColor(30, 30, 35); break;
            default:                          shirtColor = QColor(30, 58, 138); break;
        }

        p.fillRect(inner, shirtColor);
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        for (int i = inner.left() - inner.height(); i < inner.right() + inner.height(); i += 8) {
            p.drawLine(i, inner.top(), i - inner.height(), inner.bottom());
        }
        p.restore();
        return;
    }

    bool isRed = (card->suit == Hearts || card->suit == Diamonds);
    p.setPen(isRed ? QColor(220, 38, 38) : QColor(17, 24, 39));

    static const QString suitsStr[] = { "♥", "♦", "♣", "♠" };
    static const QString ranksStr[] = { "", "", "2", "3", "4", "5", "6", "7", "8", "9", "10", getLocalizedText("В", "J"), getLocalizedText("Д", "Q"), getLocalizedText("К", "K"), getLocalizedText("Т", "A") };

    QString suitTxt = suitsStr[card->suit];
    QString rankTxt = ranksStr[card->rank];

    // Масштабируемый шрифт углового индекса
    int cornerFontSize = qMax(7, qRound(rect.height() * 0.11));
    p.setFont(QFont(p.font().family(), cornerFontSize, QFont::Bold));
    p.drawText(rect.adjusted(4, 2, -2, -2), Qt::AlignTop | Qt::AlignLeft, rankTxt + "\n" + suitTxt);

    p.save();
    p.translate(rect.center());
    p.rotate(180);
    p.setFont(QFont(p.font().family(), cornerFontSize, QFont::Bold));
    QRectF localRect(-rect.width() / 2.0, -rect.height() / 2.0, rect.width(), rect.height());
    p.drawText(localRect.adjusted(4, 2, -2, -2), Qt::AlignTop | Qt::AlignLeft, rankTxt + "\n" + suitTxt);
    p.restore();

    // Масштабируемый шрифт масти в центре
    int centerFontSize = qMax(12, qRound(rect.height() * 0.32));
    p.setFont(QFont(p.font().family(), centerFontSize));
    p.drawText(rect, Qt::AlignCenter, suitTxt);

    p.restore();
}

// ============================================================================
// MAIN MENU WIDGET
// ============================================================================

MainMenuWidget::MainMenuWidget(QWidget* parent) : QWidget(parent) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Включаем нативный кинетический тач-скролл пальцем по всему экрану
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QScroller::grabGesture(scrollArea->viewport(), QScroller::TouchGesture);

    // Настройка чувствительности скролла, чтобы тапы по ComboBox не блокировались
    auto* scroller = QScroller::scroller(scrollArea->viewport());
    QScrollerProperties sp = scroller->scrollerProperties();
    sp.setScrollMetric(QScrollerProperties::DragStartDistance, 0.008); // 8 мм порог до начала скролла
    sp.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0.0);
    scroller->setScrollerProperties(sp);
#else
    QScroller::grabGesture(scrollArea->viewport(), QScroller::LeftMouseButtonGesture);
#endif

    // Стильный тонкий скроллбар в золотом стиле Royal Card Club
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

    QString frameStyle = "QFrame#panel { background: rgba(20, 27, 44, 0.88); border-radius: 14px; border: 1px solid rgba(251, 191, 36, 0.25); }";
    QString comboStyle = "QComboBox { padding: 4px 10px; border-radius: 8px; background: #182234; color: #F9FAFB; border: 1px solid #374151; } "
    "QComboBox:hover { border: 1px solid #F59E0B; } "
    "QComboBox::drop-down { border: none; width: 28px; }";
    QString inputStyle = "QLineEdit { padding: 4px 10px; border-radius: 8px; background: #182234; color: #F9FAFB; border: 1px solid #374151; } "
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
    // Плавный сбалансированный масштаб меню
    qreal s = std::clamp(std::min(width() / 1100.0, height() / 720.0), 0.6, 1.4);

    lblTitle->setFont(QFont(font().family(), qMax(18, qRound(38 * s)), QFont::Black));
    lblSub->setFont(QFont(font().family(), qMax(8, qRound(11 * s)), QFont::Bold));

    int panelW = qRound(480 * s);
    selectFrame->setFixedWidth(panelW);
    botFrame->setFixedWidth(panelW);
    netFrame->setFixedWidth(panelW);
    btnSettings->setFixedWidth(panelW);
    btnRules->setFixedWidth(panelW);

    int itemH = qRound(38 * s);
    QFont fHeader(font().family(), qMax(9, qRound(12 * s)), QFont::Bold);
    QFont fNorm(font().family(), qMax(9, qRound(12 * s)), QFont::Bold);

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
    QRadialGradient bg(width() / 2, height() / 2, qMax(width(), height()) * 0.65);
    bg.setColorAt(0.0, QColor(28, 42, 68));
    bg.setColorAt(0.5, QColor(14, 21, 35));
    bg.setColorAt(1.0, QColor(6, 9, 16));
    p.fillRect(rect(), bg);
#endif

    // Динамический размер и точное позиционирование фоновых мастей относительно панелей
    int suitSize = qBound(120, qRound(height() * 0.22), 260);
    int currentPanelW = selectFrame ? selectFrame->width() : qMin(500, int(width() * 0.8));
    double leftX = (width() - currentPanelW) / 2.0 - suitSize * 0.55;
    double rightX = (width() + currentPanelW) / 2.0 + suitSize * 0.55;

    auto drawWatermark = [&](double x, double y, const QString& suit, double angle) {
        p.save();
        p.translate(x, y);
        p.rotate(angle);
        // Используем загруженный шрифт Noto Sans
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
        showAndroidKeyboard();
    }
#endif
    return QWidget::eventFilter(watched, event);
}

// ============================================================================
// POKER WIDGET
// ============================================================================

PokerWidget::PokerWidget(NetworkManager* netMgr, QWidget* parent)
: BaseTableWidget(parent), netManager(netMgr)
{
    btnFold          = new QPushButton(getLocalizedText("FOLD", "FOLD"), this);
    btnCall          = new QPushButton(getLocalizedText("CALL", "CALL"), this);
    btnRaise         = new QPushButton(getLocalizedText("RAISE", "RAISE"), this);
    btnStartNetGame  = new QPushButton(getLocalizedText("НАЧАТЬ СЕТЕВУЮ ИГРУ", "START NETWORK GAME"), this);

    raiseSlider    = new QSlider(Qt::Horizontal, this);
    lblRaiseAmount = new QLabel("$0", this);

    QString btnStyle = "QPushButton { background: %1; color: white; font-weight: bold; border-radius: 8px; border: none; } QPushButton:hover { background: %2; }";
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
            int snapped = (val / 10) * 10;
            if (snapped != val) {
                raiseSlider->setValue(snapped);
                return;
            }
        }
        lblRaiseAmount->setText(QString("$%1").arg(val));
    });

    connect(btnFold, &QPushButton::clicked, this, [this](){ onPlayerAction("FOLD"); });
    connect(btnCall, &QPushButton::clicked, this, [this](){ onPlayerAction("CALL"); });
    connect(btnRaise, &QPushButton::clicked, this, [this]() {
        if (engine.currentTurnIdx >= engine.players.size()) return;
        Player& p = engine.players[engine.myIdx];
        int minR = engine.currentHighestBet + engine.minRaise;
        int maxR = p.balance + p.currentBet;

        if (!raiseSlider->isVisible() || minR >= maxR) {
            onPlayerAction("RAISE", maxR);
        } else {
            onPlayerAction("RAISE", raiseSlider->value());
        }
    });

    connect(btnStartNetGame, &QPushButton::clicked, this, [this]() {
        if (netManager && netManager->isHost && netManager->getActiveClientCount() >= 1) {
            int gType = netManager->gameType;
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
                int activeClients = netManager->getActiveClientCount();
                if (activeClients == 0) {
                    netManager->isLobby = true;
                    engine.gameOver = false;
                    engine.players.resize(1);
                    engine.players[0].id = 0;
                    engine.players[0].name = AppSettings::instance().nickname;
                    engine.players[0].avatar = static_cast<int>(AppSettings::instance().avatar);
                    engine.communityCards.clear();
                    engine.pot = 0;

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
            // Одиночная игра (с Ботами)
            bool isSoloHumanBankrupt = (engine.myIdx < engine.players.size() && engine.players[engine.myIdx].balance <= 0);
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
            bool isSoloHumanBankrupt = (engine.myIdx < engine.players.size() && engine.players[engine.myIdx].balance <= 0);
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
    if (netManager && senderId >= engine.players.size()) {
        while (engine.players.size() <= senderId) {
            Player p;
            p.id = engine.players.size();
            p.name = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].name : QString(getLocalizedText("Игрок %1", "Player %1")).arg(p.id + 1);
            p.avatar = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].avatar : 0;
            p.isBot = false;
            p.hasFolded = true; // Зритель до следующей раздачи
            p.balance = PokerConfig::DEFAULT_BALANCE;
            engine.players.append(p);
        }
    }
    QString act = json["action"].toString();
    if (act != "JOIN") {
        int amt = json["amount"].toInt(0);
        engine.processAction(senderId, act, amt);
    }
    broadcastNetState();
}


void PokerWidget::broadcastNetState() {
    if (!netManager || !netManager->isHost) return;
    for (int i = 0; i < netManager->clientSockets.size(); ++i) {
        auto* socket = netManager->clientSockets[i];
        int targetPlayerId = i + 1;
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
    bool isLobby = (netManager && netManager->isNetworkGame && netManager->isLobby);
    bool isMyTurn = (!engine.gameOver && !isLobby && engine.currentTurnIdx == engine.myIdx);

    btnFold->setVisible(isMyTurn);
    btnCall->setVisible(isMyTurn);
    btnRaise->setVisible(isMyTurn);
    raiseSlider->setVisible(isMyTurn);
    lblRaiseAmount->setVisible(isMyTurn);

    int activeClients = netManager ? netManager->getActiveClientCount() : 0;
    btnStartNetGame->setVisible(netManager && netManager->isHost && isLobby && activeClients >= 1);

    bool isError = engine.statusMessage.contains(getLocalizedText("Ошибка", "Error")) || engine.statusMessage.contains(getLocalizedText("потеряна", "lost"));

    bool canShowNextHand = false;
    if (!isError && engine.gameOver) {
        if (netManager && netManager->isNetworkGame) {
            if (netManager->isHost && !netManager->isLobby) {
                canShowNextHand = true;
            }
        } else {
            if (engine.players.size() >= 2) {
                canShowNextHand = true;
            }
        }
    }

    btnNextHand->setVisible(canShowNextHand);

    if (canShowNextHand) {
        if (netManager && netManager->isNetworkGame && netManager->isHost) {
            if (activeClients == 0) {
                btnNextHand->setText(getLocalizedText("ВЕРНУТЬСЯ В ЛОББИ", "RETURN TO LOBBY"));
            } else if (engine.countSolventPlayers() < 2) {
                btnNextHand->setText(getLocalizedText("ИГРАТЬ ЗАНОВО", "PLAY AGAIN"));
            } else {
                btnNextHand->setText(getLocalizedText("СЛЕДУЮЩАЯ РАЗДАЧА", "NEXT HAND"));
                if (!autoNextHandTimer->isActive() && AppSettings::instance().autoNextHand) {
                    autoNextHandTimer->start(3000);
                }
            }
        } else {
            bool isSoloHumanBankrupt = (engine.myIdx < engine.players.size() && engine.players[engine.myIdx].balance <= 0);
            if (isSoloHumanBankrupt || engine.countSolventPlayers() < 2) {
                btnNextHand->setText(getLocalizedText("ИГРАТЬ ЗАНОВО", "PLAY AGAIN"));
            } else {
                btnNextHand->setText(getLocalizedText("СЛЕДУЮЩАЯ РАЗДАЧА", "NEXT HAND"));
                if (!autoNextHandTimer->isActive() && AppSettings::instance().autoNextHand) {
                    autoNextHandTimer->start(3000);
                }
            }
        }
    } else {
        autoNextHandTimer->stop();
    }

    if (isMyTurn && engine.myIdx < engine.players.size()) {
        Player& p = engine.players[engine.myIdx];
        int toCall = engine.currentHighestBet - p.currentBet;

        if (toCall == 0) btnCall->setText(getLocalizedText("CHECK", "CHECK"));
        else btnCall->setText(QString(getLocalizedText("CALL $%1", "CALL $%1")).arg(toCall));

        int minR = engine.currentHighestBet + engine.minRaise;
        if (minR > p.balance + p.currentBet) minR = p.balance + p.currentBet;
        int maxR = p.balance + p.currentBet;

        if (minR >= maxR) {
            raiseSlider->setVisible(false);
            lblRaiseAmount->setVisible(false);
            btnRaise->setText(getLocalizedText("ALL IN", "ALL IN"));
        } else {
            raiseSlider->setRange(minR, maxR);
            raiseSlider->setSingleStep(10);
            raiseSlider->setPageStep(10);
            raiseSlider->setValue(minR);
            lblRaiseAmount->setText(QString("$%1").arg(minR));
            btnRaise->setText(getLocalizedText("RAISE", "RAISE"));
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
    qreal s = getScale();

    int btnW = qRound(110 * s);
    int btnH = qRound(44 * s);
    int sliderW = qRound(120 * s);
    int btnY = height() - btnH - qRound(18 * s);

    btnFold->setGeometry(width() / 2 - btnW * 2 - qRound(15 * s), btnY, btnW, btnH);
    btnCall->setGeometry(width() / 2 - btnW - qRound(5 * s), btnY, btnW, btnH);
    raiseSlider->setGeometry(width() / 2 + qRound(5 * s), btnY, sliderW, qRound(18 * s));
    lblRaiseAmount->setGeometry(width() / 2 + qRound(5 * s), btnY + qRound(20 * s), sliderW, qRound(22 * s));
    btnRaise->setGeometry(width() / 2 + sliderW + qRound(15 * s), btnY, btnW, btnH);

    QFont btnFont(font().family(), qMax(8, qRound(12 * s)), QFont::Bold);
    btnFold->setFont(btnFont);
    btnCall->setFont(btnFont);
    btnRaise->setFont(btnFont);
    lblRaiseAmount->setFont(btnFont);

    int startNetW = qRound(260 * s);
    int startNetH = qRound(50 * s);
    btnStartNetGame->setGeometry(width() / 2 - startNetW / 2, height() / 2 + qRound(40 * s), startNetW, startNetH);
    btnStartNetGame->setFont(btnFont);
}

void PokerWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    drawTableFelt(p);

    qreal s = getScale();

    if (netManager && netManager->isNetworkGame && netManager->isLobby) {
        p.setPen(QColor(255, 215, 0));
        p.setFont(QFont(font().family(), qMax(12, qRound(22 * s)), QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, lblStatus->text());
        return;
    }

    if (!engine.players.isEmpty()) {
        // Банк (POT)
        p.setPen(QColor(252, 211, 77));
        p.setFont(QFont(font().family(), qMax(11, qRound(17 * s)), QFont::Bold));
        int potY = height() / 2 - qRound(95 * s);
        p.drawText(QRect(0, potY, width(), qRound(28 * s)), Qt::AlignCenter, QString("POT: $%1").arg(engine.pot));

        // Общие карты на столе (Flop, Turn, River)
        int cardW = qRound(80 * s);
        int cardH = qRound(115 * s);
        int stepX = qRound(88 * s);
        int commStartX = width() / 2 - (5 * stepX) / 2;
        int commY = height() / 2 - cardH / 2;

        for (int i = 0; i < 5; ++i) {
            QRect cRect(commStartX + i * stepX, commY, cardW, cardH);
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
    int numPlayers = engine.players.size();
    qreal s = getScale();

    // Позиции мест с масштабированными отступами
    int bottomOffset = qRound(115 * s);
    int topOffset = qRound(80 * s);
    QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), bottomOffset, topOffset);

    int boxW = qRound(170 * s);
    int boxH = qRound(50 * s);

    for (int i = 0; i < numPlayers; ++i) {
        int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        QPoint pos = seatPos[displayIdx];
        auto& plr = engine.players[i];

        // 1. Подсветка активного хода
        if (engine.currentTurnIdx == i && !engine.gameOver && !plr.hasFolded && !plr.isBankrupt && !plr.isDisconnected) {
            p.setPen(QPen(QColor(59, 130, 246, 220), qMax(2, qRound(3 * s))));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(pos.x() - boxW / 2 - 4, pos.y() - boxH / 2 - 4, boxW + 8, boxH + 8, 8, 8);
        }

        // 2. Плашка игрока
        p.setBrush(QColor(15, 25, 35, 230));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.drawRoundedRect(pos.x() - boxW / 2, pos.y() - boxH / 2, boxW, boxH, 6, 6);

        // 3. Кнопка Дилера (D)
        if (engine.dealerIdx == i) {
            int dSize = qRound(18 * s);
            p.setBrush(Qt::white);
            p.setPen(QPen(Qt::black, 1));
            p.drawEllipse(pos.x() - boxW / 2 - dSize / 2, pos.y() - dSize / 2, dSize, dSize);
            p.setPen(Qt::black);
            p.setFont(QFont(font().family(), qMax(7, qRound(10 * s)), QFont::Bold));
            p.drawText(QRect(pos.x() - boxW / 2 - dSize / 2, pos.y() - dSize / 2, dSize, dSize), Qt::AlignCenter, "D");
        }

        // 4. Имя и аватар
        p.setPen(Qt::white);
        p.setFont(QFont(font().family(), qMax(8, qRound(11 * s)), QFont::Bold));
        QString nameWithAvatar = getAvatarEmojiById(plr.avatar) + " " + plr.name;
        p.drawText(QRect(pos.x() - boxW / 2 + 8, pos.y() - boxH / 2 + 3, boxW - 16, boxH / 2), Qt::AlignLeft | Qt::AlignVCenter, nameWithAvatar);

        // 5. Подсказка комбинации (для локального игрока снизу)
        if (displayIdx == 0 && AppSettings::instance().showPokerHandHint && !plr.hasFolded && !plr.holeCards.isEmpty() && engine.phase != PREFLOP) {
            QVector<Card> allCards = plr.holeCards;
            allCards.append(engine.communityCards);
            HandValue hv = evaluate7Cards(allCards);

            p.setPen(QColor(251, 191, 36));
            p.setFont(QFont(font().family(), qMax(8, qRound(10 * s)), QFont::Bold));
            p.drawText(QRect(pos.x() - boxW, pos.y() + boxH / 2 + 2, boxW * 2, qRound(20 * s)), Qt::AlignCenter, QString("[%1]").arg(hv.name));
        }

        // 6. Баланс
        p.setPen(QColor(167, 243, 208));
        p.setFont(QFont(font().family(), qMax(8, qRound(10 * s)), QFont::Bold));
        p.drawText(QRect(pos.x() - boxW / 2 + 8, pos.y(), boxW - 16, boxH / 2), Qt::AlignLeft | Qt::AlignVCenter, QString("$%1").arg(plr.balance));

        // 7. Текущая ставка
        if (plr.currentBet > 0) {
            p.setPen(QColor(253, 230, 138));
            p.drawText(QRect(pos.x() - boxW / 2 + 8, pos.y(), boxW - 16, boxH / 2), Qt::AlignRight | Qt::AlignVCenter, QString("Bet: %1").arg(plr.currentBet));
        }

        // 8. Статусы (FOLD, ALL-IN, DISCONNECTED)
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

        // 9. Карманные карты (Hole Cards)
        if (!plr.hasFolded && !plr.isBankrupt && !plr.isDisconnected) {
            int cardGap = qRound(8 * s);
            int totalHoleW = plr.holeCards.size() * cardW + (plr.holeCards.size() - 1) * cardGap;
            int cardsStartX = pos.x() - totalHoleW / 2;
            int cardsY = (displayIdx == 0) ? pos.y() - cardH - qRound(32 * s) : pos.y() + boxH / 2 + qRound(6 * s);

            for (int c = 0; c < plr.holeCards.size(); ++c) {
                bool faceUp = (i == engine.myIdx || engine.phase == SHOWDOWN || engine.gameOver);
                QRect cRect(cardsStartX + c * (cardW + cardGap), cardsY, cardW, cardH);
                drawCard(p, cRect, &plr.holeCards[c], faceUp);
            }
        }
    }
}

// ============================================================================
// DURAK WIDGET
// ============================================================================

DurakWidget::DurakWidget(NetworkManager* netMgr, QWidget* parent) : BaseTableWidget(parent), netManager(netMgr) {
    btnPass = new QPushButton(getLocalizedText("БИТО / ПАС", "DONE / PASS"), this);
    btnTake = new QPushButton(getLocalizedText("ВЗЯТЬ КАРТЫ", "TAKE CARDS"), this);

    btnPass->setStyleSheet("QPushButton { background: #3B82F6; color: white; font-weight: bold; border-radius: 6px; padding: 10px; }");
    btnTake->setStyleSheet("QPushButton { background: #F59E0B; color: white; font-weight: bold; border-radius: 6px; padding: 10px; }");

    btnPass->setCursor(Qt::PointingHandCursor);
    btnTake->setCursor(Qt::PointingHandCursor);

    connect(btnPass, &QPushButton::clicked, this, [this]() {
        if (netManager && netManager->isNetworkGame && !netManager->isHost) {
            QJsonObject json; json["act"] = "PASS";
            netManager->sendJsonToServer(json);
        } else {
            engine.passAction();
            if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
        }
        updateUI();
    });

    connect(btnTake, &QPushButton::clicked, this, [this]() {
        if (netManager && netManager->isNetworkGame && !netManager->isHost) {
            QJsonObject json; json["act"] = "TAKE";
            netManager->sendJsonToServer(json);
        } else {
            engine.takeAction();
            if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
        }
        updateUI();
    });

    connect(btnNextHand, &QPushButton::clicked, this, [this]() {
        if (netManager && netManager->isNetworkGame && netManager->isHost) {
            int activeClients = netManager->getActiveClientCount();
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
    connect(aiTimer, &QTimer::timeout, this, &DurakWidget::handleAiLogic);
    connect(&engine, &DurakEngine::stateChanged, this, &DurakWidget::updateUI);
}

void DurakWidget::startSingleGame(int botCount) {
    if (netManager) {
        netManager->disconnectAll();
    }
    engine.initGame(botCount);
    updateUI();
    aiTimer->start(700);
}

void DurakWidget::handleAiLogic() {
    if (parentWidget() && static_cast<QStackedWidget*>(parentWidget())->currentIndex() != 2) return;
    if (engine.isProcessingMove) return;

    if (!engine.gameOver && (!netManager || !netManager->isNetworkGame || netManager->isHost)) {
        if (engine.makeAiMove()) {
            if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
            updateUI();
        }
    }
}

void DurakWidget::updateUI() {
    bool canPressBito = false;
    bool canPressTake = false;
    bool allDefended = true;
    int undefendedCount = 0;

    if (!engine.table.isEmpty()) {
        for (const auto& pair : engine.table) {
            if (!pair.isDefended) {
                allDefended = false;
                undefendedCount++;
            }
        }

        int defenderCardsCount = engine.players[engine.defenderIdx].hand.size();
        bool limitReached = (undefendedCount >= defenderCardsCount || engine.table.size() >= 6);

        if (engine.isDefenderTaking) {
            if (!limitReached && !engine.players[engine.myIdx].hand.isEmpty() &&
                (engine.attackerIdx == engine.myIdx || engine.currentTurnIdx == engine.myIdx)) {
                canPressBito = true;
                }
        } else {
            if (allDefended && engine.attackerIdx == engine.myIdx && !engine.players[engine.myIdx].hand.isEmpty()) {
                canPressBito = true;
            }
            if (!allDefended && engine.defenderIdx == engine.myIdx) {
                canPressTake = true;
            }
        }
    }

    int activeClients = netManager ? netManager->getActiveClientCount() : 0;
    bool isHostOrSolo = (!netManager || !netManager->isNetworkGame || netManager->isHost);

    btnPass->setVisible(canPressBito && !engine.gameOver);
    btnTake->setVisible(canPressTake && !engine.gameOver);
    btnNextHand->setVisible(engine.gameOver && isHostOrSolo);

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
    } else if (engine.myIdx < engine.players.size() && engine.players[engine.myIdx].isOut && !engine.deck.isEmpty()) {
        lblStatus->setText(getLocalizedText("Вы зашли во время игры. Ожидание следующего раунда...", "You joined mid-game. Waiting for next round..."));
    } else if (engine.isDefenderTaking) {
        if (engine.defenderIdx == engine.myIdx) {
            lblStatus->setText(getLocalizedText("Вы берёте карты! Ожидание завершения хода...", "You take cards! Waiting for turn to finish..."));
        } else if (engine.players[engine.myIdx].hand.isEmpty()) {
            lblStatus->setText(QString(getLocalizedText("%1 берёт карты!", "%1 takes cards!")).arg(engine.players[engine.defenderIdx].name));
        } else if (engine.attackerIdx == engine.myIdx || engine.currentTurnIdx == engine.myIdx) {
            lblStatus->setText(QString(getLocalizedText("%1 берёт карты! Подкиньте или нажмите Пас", "%1 takes cards! Toss cards or press Pass")).arg(engine.players[engine.defenderIdx].name));
        } else {
            lblStatus->setText(QString(getLocalizedText("%1 берёт карты!", "%1 takes cards!")).arg(engine.players[engine.defenderIdx].name));
        }
    } else if (engine.table.isEmpty()) {
        if (engine.attackerIdx == engine.myIdx) {
            lblStatus->setText(getLocalizedText("Ваш ход! Атакуйте!", "Your turn! Attack!"));
        } else if (engine.defenderIdx == engine.myIdx) {
            lblStatus->setText(QString(getLocalizedText("Ожидание атаки от игрока %1...", "Waiting for %1 to attack...")).arg(engine.players[engine.attackerIdx].name));
        } else {
            lblStatus->setText(QString(getLocalizedText("Ход игрока %1", "%1's turn")).arg(engine.players[engine.attackerIdx].name));
        }
    } else {
        if (!allDefended) {
            if (engine.defenderIdx == engine.myIdx) {
                lblStatus->setText(getLocalizedText("Ваш ход! Защищайтесь!", "Your turn! Defend!"));
            } else if (engine.attackerIdx == engine.myIdx) {
                lblStatus->setText(QString(getLocalizedText("Ожидание защиты от игрока %1...", "Waiting for %1 to defend...")).arg(engine.players[engine.defenderIdx].name));
            } else {
                lblStatus->setText(QString(getLocalizedText("%1 защищается...", "%1 is defending...")).arg(engine.players[engine.defenderIdx].name));
            }
        } else {
            if (engine.attackerIdx == engine.myIdx) {
                lblStatus->setText(getLocalizedText("Все карты отбиты! Подкиньте или нажмите Бито", "All cards beaten! Toss cards or press Done"));
            } else if (engine.defenderIdx == engine.myIdx) {
                lblStatus->setText(QString(getLocalizedText("Ожидание хода %1 (подкинет или Бито)...", "Waiting for %1 (toss or Done)...")).arg(engine.players[engine.attackerIdx].name));
            } else {
                lblStatus->setText(QString(getLocalizedText("%1 подкидывает или Бито...", "%1 is tossing or Done...")).arg(engine.players[engine.attackerIdx].name));
            }
        }
    }

    update();
}

void DurakWidget::processNetAction(int senderId, const QJsonObject& json) {
    if (netManager && senderId >= engine.players.size()) {
        while (engine.players.size() <= senderId) {
            DurakPlayer p;
            p.id = engine.players.size();
            p.name = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].name : QString(getLocalizedText("Игрок %1", "Player %1")).arg(p.id + 1);
            p.avatar = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].avatar : 0;
            p.isBot = false;
            p.isOut = true;
            engine.players.append(p);
        }
    }

    QString act = json["act"].toString();
    if (act == "ATTACK") {
        engine.playAttackCard(senderId, json["cardHandIdx"].toInt());
    } else if (act == "DEFEND") {
        engine.playDefendCard(senderId, json["cardHandIdx"].toInt(), json["tableIdx"].toInt());
    } else if (act == "PASS") {
        engine.passAction();
    } else if (act == "TAKE") {
        engine.takeAction();
    }
    broadcastNetState();
}

void DurakWidget::broadcastNetState() {
    if (!netManager || !netManager->isHost) return;
    for (int i = 0; i < netManager->clientSockets.size(); ++i) {
        auto* socket = netManager->clientSockets[i];
        int targetPlayerId = i + 1;
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            QJsonObject json = engine.toJson(targetPlayerId);
            json["isLobby"]  = false;
            json["gameType"] = 1;
            netManager->sendJsonToClient(i, json);
        }
    }
    updateUI();
}

void DurakWidget::resizeEvent(QResizeEvent* ev) {
    BaseTableWidget::resizeEvent(ev);
    qreal s = getScale();

    int btnW = qRound(120 * s);
    int btnH = qRound(45 * s);
    int btnY = height() - btnH - qRound(18 * s);

    btnPass->setGeometry(width() - btnW * 2 - qRound(25 * s), btnY, btnW, btnH);
    btnTake->setGeometry(width() - btnW - qRound(15 * s), btnY, btnW, btnH);

    QFont btnFont(font().family(), qMax(8, qRound(12 * s)), QFont::Bold);
    btnPass->setFont(btnFont);
    btnTake->setFont(btnFont);
}

void DurakWidget::mouseMoveEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty() || engine.myIdx >= engine.players.size()) return;
    auto& myHand = engine.players[engine.myIdx].hand;
    qreal s = getScale();

    int cardW = qRound(80 * s);
    int cardH = qRound(115 * s);
    int handY = height() - cardH - qRound(110 * s);
    int stepX = qMin(qRound(50 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    int newHovered = -1;
    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = (i == selectedHandCardIdx) ? qRound(-25 * s) : ((i == hoveredHandCardIdx) ? qRound(-12 * s) : 0);
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

void DurakWidget::mousePressEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty() || engine.myIdx >= engine.players.size()) return;
    auto& myHand = engine.players[engine.myIdx].hand;
    qreal s = getScale();

    int cardW = qRound(80 * s);
    int cardH = qRound(115 * s);
    int handY = height() - cardH - qRound(110 * s);
    int stepX = qMin(qRound(50 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = (i == selectedHandCardIdx) ? qRound(-25 * s) : ((i == hoveredHandCardIdx) ? qRound(-12 * s) : 0);
        if (QRect(startX + i * stepX, handY + offsetY, cardW, cardH).contains(ev->pos())) {
            bool canAttack = (engine.attackerIdx == engine.myIdx) ||
            (engine.isDefenderTaking && engine.currentTurnIdx == engine.myIdx);

            if (canAttack) {
                selectedHandCardIdx = i;
                if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                    QJsonObject json; json["act"] = "ATTACK"; json["cardHandIdx"] = selectedHandCardIdx;
                    netManager->sendJsonToServer(json);
                    selectedHandCardIdx = -1;
                } else {
                    if (engine.playAttackCard(engine.myIdx, selectedHandCardIdx)) {
                        selectedHandCardIdx = -1;
                        if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
                    }
                }
                updateUI();
                return;
            }

            if (engine.defenderIdx == engine.myIdx && !engine.isDefenderTaking) {
                if (selectedHandCardIdx == i) {
                    selectedHandCardIdx = -1;
                } else {
                    selectedHandCardIdx = i;
                    QVector<int> undefended;
                    for (int t = 0; t < engine.table.size(); ++t) {
                        if (!engine.table[t].isDefended) undefended.append(t);
                    }

                    if (undefended.size() == 1) {
                        int targetTableIdx = undefended.first();
                        if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                            QJsonObject json; json["act"] = "DEFEND"; json["cardHandIdx"] = selectedHandCardIdx; json["tableIdx"] = targetTableIdx;
                            netManager->sendJsonToServer(json);
                            selectedHandCardIdx = -1;
                        } else {
                            if (engine.playDefendCard(engine.myIdx, selectedHandCardIdx, targetTableIdx)) {
                                selectedHandCardIdx = -1;
                                if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
                            }
                        }
                    }
                }
                updateUI();
                return;
            }
            updateUI();
            return;
        }
    }

    if (selectedHandCardIdx != -1 && engine.defenderIdx == engine.myIdx && !engine.isDefenderTaking) {
        int tableY = height() / 2 - cardH / 2;
        int stepTableX = qRound(110 * s);
        int totalTableW = engine.table.size() * stepTableX;
        int tableStartX = (width() - totalTableW) / 2;
        for (int t = 0; t < engine.table.size(); ++t) {
            if (QRect(tableStartX + t * stepTableX, tableY, cardW, cardH).contains(ev->pos()) && !engine.table[t].isDefended) {
                if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                    QJsonObject json; json["act"] = "DEFEND"; json["cardHandIdx"] = selectedHandCardIdx; json["tableIdx"] = t;
                    netManager->sendJsonToServer(json);
                    selectedHandCardIdx = -1;
                } else {
                    if (engine.playDefendCard(engine.myIdx, selectedHandCardIdx, t)) {
                        selectedHandCardIdx = -1;
                        if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
                    }
                }
                updateUI();
                return;
            }
        }
    }
}

void DurakWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawTableFelt(p);

    qreal s = getScale();
    int leftOffset = getSafeLeftMargin();

    if (!engine.players.isEmpty()) {
        int cardW = qRound(80 * s);
        int cardH = qRound(115 * s);

        if (!engine.deck.isEmpty()) {
            drawCard(p, QRect(leftOffset, qRound(125 * s), cardH, cardW), &engine.trumpCard, true);
            drawCard(p, QRect(leftOffset + qRound(40 * s), qRound(95 * s), cardW, cardH), nullptr, false);

            p.setPen(Qt::white);
            p.setFont(QFont(font().family(), qMax(8, qRound(11 * s)), QFont::Bold));
            p.drawText(leftOffset + qRound(20 * s), qRound(225 * s), QString(getLocalizedText("Карт: %1", "Cards: %1")).arg(engine.deck.size()));
        } else {
            p.setPen(QColor(255, 235, 59));
            p.setFont(QFont(font().family(), qMax(9, qRound(13 * s)), QFont::Bold));
            static const QString suitsStr[] = { "♥", "♦", "♣", "♠" };
            p.drawText(leftOffset + qRound(20 * s), qRound(120 * s), QString(getLocalizedText("Козырь: %1", "Trump: %1")).arg(suitsStr[engine.trumpCard.suit]));
        }

        if (engine.bitoCount > 0) {
            p.save();
            p.translate(width() - qRound(80 * s), qRound(150 * s));
            p.rotate(15);
            drawCard(p, QRect(-cardW / 2, -cardH / 2, cardW, cardH), nullptr, false);
            p.restore();

            p.setPen(Qt::white);
            p.setFont(QFont(font().family(), qMax(8, qRound(11 * s)), QFont::Bold));
            p.drawText(width() - qRound(110 * s), qRound(225 * s), QString(getLocalizedText("Бито: %1", "Discards: %1")).arg(engine.bitoCount));
        }

        drawPlayers(p, cardW, cardH);

        int tableY = height() / 2 - cardH / 2;
        int stepX = qRound(110 * s);
        int totalTableW = engine.table.size() * stepX;
        int tableStartX = (width() - totalTableW) / 2;

        for (int t = 0; t < engine.table.size(); ++t) {
            int cardX = tableStartX + t * stepX;
            drawCard(p, QRect(cardX, tableY, cardW, cardH), &engine.table[t].attack, true);
            if (engine.table[t].isDefended) {
                drawCard(p, QRect(cardX + qRound(22 * s), tableY + qRound(22 * s), cardW, cardH), &engine.table[t].defend, true);
            }
        }

        if (engine.myIdx < engine.players.size()) {
            auto& myHand = engine.players[engine.myIdx].hand;
            int handY = height() - cardH - qRound(110 * s);
            int stepHandX = qMin(qRound(50 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
            int startX = (width() - (myHand.size() * stepHandX + (cardW - stepHandX))) / 2;
            for (int i = 0; i < myHand.size(); ++i) {
                bool isSelected = (i == selectedHandCardIdx);
                bool isHovered  = (i == hoveredHandCardIdx);
                int offsetY     = isSelected ? qRound(-25 * s) : (isHovered ? qRound(-12 * s) : 0);
                drawCard(p, QRect(startX + i * stepHandX, handY + offsetY, cardW, cardH), &myHand[i], true, isSelected);
            }
        }
    }

    if (engine.gameOver) {
        drawGameOverBanner(p, engine.statusMessage);
    }
}

void DurakWidget::drawPlayers(QPainter& p, int cardW, int cardH) {
    int numPlayers = engine.players.size();
    qreal s = getScale();
    QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), qRound(75 * s), qRound(80 * s));

    int boxW = qRound(160 * s);
    int boxH = qRound(42 * s);

    for (int i = 0; i < numPlayers; ++i) {
        int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        QPoint pos = seatPos[displayIdx];
        auto& opp = engine.players[i];

        if ((engine.attackerIdx == i || engine.defenderIdx == i) && !engine.gameOver) {
            p.setPen(QPen(QColor(59, 130, 246, 220), qMax(2, qRound(3 * s))));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(pos.x() - boxW / 2 - 4, pos.y() - boxH / 2 - 4, boxW + 8, boxH + 8, 8, 8);
        }

        p.setBrush(QColor(15, 25, 35, 230));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.drawRoundedRect(pos.x() - boxW / 2, pos.y() - boxH / 2, boxW, boxH, 6, 6);

        p.setPen(Qt::white);
        p.setFont(QFont(font().family(), qMax(8, qRound(11 * s)), QFont::Bold));
        QString nameWithAvatar = getAvatarEmojiById(opp.avatar) + " " + opp.name;
        p.drawText(QRect(pos.x() - boxW / 2, pos.y() - boxH / 2, boxW, boxH), Qt::AlignCenter, nameWithAvatar);

        if (displayIdx != 0) {
            int handSize = opp.hand.size();
            int oppStep = qRound(15 * s);
            int oppW = cardW - qRound(25 * s);
            int oppH = cardH - qRound(35 * s);
            int startX = pos.x() - (handSize * oppStep + (oppW - oppStep)) / 2;
            for (int c = 0; c < handSize; ++c) {
                drawCard(p, QRect(startX + c * oppStep, pos.y() + boxH / 2 + qRound(5 * s), oppW, oppH), nullptr, false);
            }
        }
    }
}

// ============================================================================
// KOZEL WIDGET
// ============================================================================

KozelWidget::KozelWidget(NetworkManager* netMgr, QWidget* parent) : BaseTableWidget(parent), netManager(netMgr) {
    btnPlayCards = new QPushButton(getLocalizedText("СДЕЛАТЬ ХОД", "PLAY CARDS"), this);
    btnPlayCards->setStyleSheet("QPushButton { background: #10B981; color: white; font-weight: bold; border-radius: 6px; padding: 6px; }");
    btnPlayCards->setCursor(Qt::PointingHandCursor);
    btnPlayCards->hide();

    connect(btnPlayCards, &QPushButton::clicked, this, [this]() {
        if (!selectedHandCardIndices.isEmpty()) {
            if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                QJsonObject json; json["act"] = "PLAY";
                QJsonArray arr;
                for (int idx : selectedHandCardIndices) arr.append(idx);
                json["cardIndices"] = arr;
                netManager->sendJsonToServer(json);
            } else {
                engine.playCards(engine.myIdx, selectedHandCardIndices);
                if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
            }
            selectedHandCardIndices.clear();
            updateUI();
        }
    });

    connect(btnNextHand, &QPushButton::clicked, this, [this]() {
        if (netManager && netManager->isNetworkGame && netManager->isHost) {
            int activeClients = netManager->getActiveClientCount();
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
    connect(aiTimer, &QTimer::timeout, this, &KozelWidget::handleAiLogic);
    connect(&engine, &KozelEngine::stateChanged, this, &KozelWidget::updateUI);
}

void KozelWidget::startSingleGame(int botCount) {
    if (netManager) {
        netManager->disconnectAll();
    }
    engine.initGame(botCount);
    updateUI();
    aiTimer->start(800);
}

void KozelWidget::handleAiLogic() {
    if (parentWidget() && static_cast<QStackedWidget*>(parentWidget())->currentIndex() != 3) return;
    if (engine.isProcessingMove) return;

    if (!engine.gameOver && (!netManager || !netManager->isNetworkGame || netManager->isHost)) {
        if (engine.makeAiMove()) {
            if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
            updateUI();
        }
    }
}

void KozelWidget::updateUI() {
    if (engine.gameOver) {
        lblStatus->setText(engine.statusMessage);
    } else if (engine.myIdx < engine.players.size() && engine.players[engine.myIdx].isOut) {
        lblStatus->setText(getLocalizedText("Вы зашли во время игры. Ожидание следующего раунда...", "You joined mid-game. Waiting for next round..."));
    } else if (engine.statusMessage.contains(getLocalizedText("забирает взятку", "takes trick"))) {
        QString winnerName = engine.statusMessage.section(getLocalizedText(" забирает взятку", " takes trick"), 0, 0);
        if (engine.myIdx < engine.players.size() && winnerName == engine.players[engine.myIdx].name) {
            int pts = engine.statusMessage.section('+', 1).section(' ', 0, 0).toInt();
            lblStatus->setText(QString(getLocalizedText("Вы забираете взятку (+%1 очков) и ходите!", "You take the trick (+%1 pts) and lead!")).arg(pts));
        } else {
            lblStatus->setText(engine.statusMessage);
        }
    } else {
        if (engine.currentTurnIdx == engine.myIdx) {
            lblStatus->setText(getLocalizedText("Ваш ход! Заходите любой картой.", "Your turn! Lead with any card."));
        } else {
            lblStatus->setText(QString(getLocalizedText("Ход игрока %1", "%1's turn")).arg(engine.players[engine.currentTurnIdx].name));
        }
    }

    if (engine.currentTurnIdx != engine.myIdx) {
        selectedHandCardIndices.clear();
    }

    int activeClients = netManager ? netManager->getActiveClientCount() : 0;
    bool isHostOrSolo = (!netManager || !netManager->isNetworkGame || netManager->isHost);

    btnPlayCards->setVisible(engine.currentTurnIdx == engine.myIdx && !selectedHandCardIndices.isEmpty() && !engine.gameOver);
    btnNextHand->setVisible(engine.gameOver && isHostOrSolo);

    if (netManager && netManager->isNetworkGame && netManager->isHost) {
        if (activeClients == 0) {
            btnNextHand->setText(getLocalizedText("ВЕРНУТЬСЯ В ЛОББИ", "RETURN TO LOBBY"));
        } else {
            btnNextHand->setText(getLocalizedText("ИГРАТЬ ЗАНОВО", "PLAY AGAIN"));
        }
    } else {
        btnNextHand->setText(getLocalizedText("ИГРАТЬ ЗАНОВО", "PLAY AGAIN"));
    }

    update();
}

void KozelWidget::processNetAction(int senderId, const QJsonObject& json) {
    if (netManager && senderId >= engine.players.size()) {
        while (engine.players.size() <= senderId) {
            KozelPlayer p;
            p.id = engine.players.size();
            p.name = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].name : QString(getLocalizedText("Игрок %1", "Player %1")).arg(p.id + 1);
            p.avatar = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].avatar : 0;
            p.isBot = false;
            engine.players.append(p);
        }
    }

    QString act = json["act"].toString();
    if (act == "PLAY") {
        QJsonArray arr = json["cardIndices"].toArray();
        QVector<int> indices;
        for (auto v : arr) indices.append(v.toInt());
        engine.playCards(senderId, indices);
    }
    broadcastNetState();
}

void KozelWidget::broadcastNetState() {
    if (!netManager || !netManager->isHost) return;
    for (int i = 0; i < netManager->clientSockets.size(); ++i) {
        auto* socket = netManager->clientSockets[i];
        int targetPlayerId = i + 1;
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            QJsonObject json = engine.toJson(targetPlayerId);
            json["isLobby"]  = false;
            json["gameType"] = 2;
            netManager->sendJsonToClient(i, json);
        }
    }
    updateUI();
}

void KozelWidget::resizeEvent(QResizeEvent* ev) {
    BaseTableWidget::resizeEvent(ev);
    qreal s = getScale();

    int btnW = qRound(150 * s);
    int btnH = qRound(45 * s);
    btnPlayCards->setGeometry(width() - btnW - qRound(20 * s), height() - btnH - qRound(18 * s), btnW, btnH);
    btnPlayCards->setFont(QFont(font().family(), qMax(8, qRound(13 * s)), QFont::Bold));
}

void KozelWidget::mouseMoveEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty() || engine.myIdx >= engine.players.size()) return;
    auto& myHand = engine.players[engine.myIdx].hand;
    qreal s = getScale();

    int cardW = qRound(80 * s);
    int cardH = qRound(115 * s);
    int handY = height() - cardH - qRound(110 * s);
    int stepX = qMin(qRound(60 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    int newHovered = -1;
    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = selectedHandCardIndices.contains(i) ? qRound(-25 * s) : ((i == hoveredHandCardIdx) ? qRound(-12 * s) : 0);
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

void KozelWidget::mousePressEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty() || engine.currentTurnIdx != engine.myIdx) return;
    auto& myHand = engine.players[engine.myIdx].hand;
    qreal s = getScale();

    int cardW = qRound(80 * s);
    int cardH = qRound(115 * s);
    int handY = height() - cardH - qRound(110 * s);
    int stepX = qMin(qRound(60 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = selectedHandCardIndices.contains(i) ? qRound(-25 * s) : ((i == hoveredHandCardIdx) ? qRound(-12 * s) : 0);
        if (QRect(startX + i * stepX, handY + offsetY, cardW, cardH).contains(ev->pos())) {
            if (selectedHandCardIndices.contains(i)) {
                if (!selectedHandCardIndices.isEmpty()) {
                    if (netManager && netManager->isNetworkGame && !netManager->isHost) {
                        QJsonObject json; json["act"] = "PLAY";
                        QJsonArray arr;
                        for (int idx : selectedHandCardIndices) arr.append(idx);
                        json["cardIndices"] = arr;
                        netManager->sendJsonToServer(json);
                    } else {
                        engine.playCards(engine.myIdx, selectedHandCardIndices);
                        if (netManager && netManager->isNetworkGame && netManager->isHost) broadcastNetState();
                    }
                    selectedHandCardIndices.clear();
                }
            } else {
                if (!selectedHandCardIndices.isEmpty()) {
                    Suit firstSuit = myHand[selectedHandCardIndices.first()].suit;
                    if (myHand[i].suit != firstSuit) selectedHandCardIndices.clear();
                }
                selectedHandCardIndices.append(i);
            }
            updateUI();
            return;
        }
    }
}

void KozelWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawTableFelt(p);

    qreal s = getScale();

    if (!engine.players.isEmpty()) {
        int cardW = qRound(80 * s);
        int cardH = qRound(115 * s);

        p.setPen(QColor(255, 215, 0));
        p.setFont(QFont(font().family(), qMax(9, qRound(14 * s)), QFont::Bold));
        static const QString suitsStr[] = { "♥", "♦", "♣", "♠" };
        p.drawText(qRound(35 * s), qRound(80 * s), QString(getLocalizedText("Козырь: %1", "Trump: %1")).arg(suitsStr[engine.trumpSuit]));

        drawPlayers(p, cardW, cardH);

        int trickStep = qRound(45 * s);
        int trickStartX = width() / 2 - (engine.currentTrick.size() * trickStep) / 2;
        for (int t = 0; t < engine.currentTrick.size(); ++t) {
            drawCard(p, QRect(trickStartX + t * trickStep, height() / 2 - cardH / 2, cardW, cardH), &engine.currentTrick[t].second, true);
        }

        if (engine.myIdx < engine.players.size()) {
            auto& myHand = engine.players[engine.myIdx].hand;
            int handY = height() - cardH - qRound(110 * s);
            int stepHandX = qMin(qRound(60 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
            int startX = (width() - (myHand.size() * stepHandX + (cardW - stepHandX))) / 2;
            for (int i = 0; i < myHand.size(); ++i) {
                bool isSelected = selectedHandCardIndices.contains(i);
                bool isHovered  = (i == hoveredHandCardIdx);
                int offsetY     = isSelected ? qRound(-25 * s) : (isHovered ? qRound(-12 * s) : 0);
                drawCard(p, QRect(startX + i * stepHandX, handY + offsetY, cardW, cardH), &myHand[i], true, isSelected);
            }
        }
    }

    if (engine.gameOver) {
        drawGameOverBanner(p, engine.statusMessage);
    }
}

void KozelWidget::drawPlayers(QPainter& p, int cardW, int cardH) {
    int numPlayers = engine.players.size();
    qreal s = getScale();
    QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), qRound(75 * s), qRound(80 * s));

    int boxW = qRound(150 * s);
    int boxH = qRound(45 * s);

    for (int i = 0; i < numPlayers; ++i) {
        int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        QPoint pos = seatPos[displayIdx];
        auto& plr = engine.players[i];

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
        QString nameWithAvatar = getAvatarEmojiById(plr.avatar) + " " + plr.name;
        p.drawText(QRect(pos.x() - boxW / 2 + 6, pos.y() - boxH / 2 + 3, boxW - 12, boxH / 2), Qt::AlignLeft, nameWithAvatar);

        p.setPen(QColor(167, 243, 208));
        p.drawText(QRect(pos.x() - boxW / 2 + 6, pos.y(), boxW - 12, boxH / 2), Qt::AlignLeft, QString(getLocalizedText("Очки: %1", "Points: %1")).arg(plr.pointsCollected));

        if (displayIdx != 0) {
            int handSize = plr.hand.size();
            int oppStep = qRound(15 * s);
            int oppW = cardW - qRound(25 * s);
            int oppH = cardH - qRound(35 * s);
            int startX = pos.x() - (handSize * oppStep + (oppW - oppStep)) / 2;
            for (int c = 0; c < handSize; ++c) {
                drawCard(p, QRect(startX + c * oppStep, pos.y() + boxH / 2 + qRound(5 * s), oppW, oppH), nullptr, false);
            }
        }
    }
}

// ============================================================================
// UNO WIDGET
// ============================================================================

UnoWidget::UnoWidget(NetworkManager* netMgr, QWidget* parent) : BaseTableWidget(parent), netManager(netMgr) {
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
                bool callUno = declaredUnoThisTurn;
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

    // Кнопка ОТМЕНЫ выбора цвета Wild-карты
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
            int activeClients = netManager->getActiveClientCount();
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

    arrowAnimTimer = new QTimer(this);
    animElapsedTimer.start();
    connect(arrowAnimTimer, &QTimer::timeout, this, [this]() {
        if (!engine.gameOver && isVisible()) {
            qreal dt = animElapsedTimer.restart() / 1000.0;
            if (dt > 0.1) dt = 0.1; // Защита от рывка при возвращении из фона

            const qreal ROTATION_SPEED_DEG_PER_SEC = 60.0; // 60 градусов в секунду (полный оборот за 6 сек)
            arrowAnimAngle += engine.direction * ROTATION_SPEED_DEG_PER_SEC * dt;

            if (arrowAnimAngle >= 360.0) arrowAnimAngle -= 360.0;
            if (arrowAnimAngle < 0.0)    arrowAnimAngle += 360.0;
            update();
        } else {
            animElapsedTimer.restart();

        }
    });
    arrowAnimTimer->start(16); // 60 FPS (каждые 16 мс)
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
    bool isMyTurn = (engine.currentTurnIdx == engine.myIdx && !engine.gameOver);
    bool hasMove = (engine.myIdx < engine.players.size()) && engine.hasPlayableCard(engine.myIdx);

    if (engine.accumulatedPenalty > 0) {
        btnDrawCard->setText(QString(getLocalizedText("ВЗЯТЬ ШТРАФ (+%1)", "TAKE PENALTY (+%1)")).arg(engine.accumulatedPenalty));
        // Кнопка штрафа всегда
        btnDrawCard->setVisible(isMyTurn); // btnDrawCard->setVisible(isMyTurn && !hasMove);
        btnPass->setVisible(false);
    } else {
        btnDrawCard->setText(getLocalizedText("ВЗЯТЬ КАРТУ", "DRAW CARD"));
        // Кнопка добора видна ТОЛЬКО если нет ни одной подходящей карты в руке
        btnDrawCard->setVisible(isMyTurn && !hasMove && (!engine.hasDrawnThisTurn || engine.drawMode == UnoDrawMode::DrawUntilMatch));
        btnPass->setVisible(isMyTurn && engine.hasDrawnThisTurn && engine.drawMode == UnoDrawMode::DrawOne);
    }

    // Обновление доступности кнопки UNO для игрока
    if (engine.myIdx < engine.players.size()) {
        int handCount = engine.players[engine.myIdx].hand.size();
        bool isVulnerable = (engine.unoVulnerablePlayerIdx == engine.myIdx);
        btnUno->setVisible((isMyTurn && handCount <= 2 && hasMove && !engine.players[engine.myIdx].saidUno) || isVulnerable);
    } else {
        btnUno->setVisible(false);
    }

    // Обновление кнопки ловли (CATCH UNO)
    bool canCatch = false;
    for (int i = 0; i < engine.players.size(); ++i) {
        if (i != engine.myIdx && (engine.unoVulnerablePlayerIdx == i || (engine.players[i].hand.size() == 1 && !engine.players[i].saidUno))) {
            canCatch = true;
            break;
        }
    }
    btnCatchUno->setVisible(canCatch && !engine.gameOver);

    int activeClients = netManager ? netManager->getActiveClientCount() : 0;
    bool isHostOrSolo = (!netManager || !netManager->isNetworkGame || netManager->isHost);
    btnNextHand->setVisible(engine.gameOver && isHostOrSolo);

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
        static const QString colNames[] = { getLocalizedText("Красный", "Red"), getLocalizedText("Жёлтый", "Yellow"), getLocalizedText("Зелёный", "Green"), getLocalizedText("Синий", "Blue") };
        QString colorTxt = colNames[engine.currentColor];
        QString penaltySuffix = (engine.accumulatedPenalty > 0) ? QString(getLocalizedText(" [ШТРАФ: +%1]", " [PENALTY: +%1]")).arg(engine.accumulatedPenalty) : "";

        if (engine.currentTurnIdx == engine.myIdx) {
            lblStatus->setText(QString(getLocalizedText("Ваш ход! Цвет: %1%2", "Your turn! Color: %1%2")).arg(colorTxt, penaltySuffix));
        } else if (engine.currentTurnIdx >= 0 && engine.currentTurnIdx < engine.players.size()) {
            lblStatus->setText(QString(getLocalizedText("Ход игрока %1 (Цвет: %2)%3", "%1's turn (Color: %2)%3")).arg(engine.players[engine.currentTurnIdx].name, colorTxt, penaltySuffix));
        }
    }

    update();
}

void UnoWidget::processNetAction(int senderId, const QJsonObject& json) {
    if (netManager && senderId >= engine.players.size()) {
        while (engine.players.size() <= senderId) {
            UnoPlayer p;
            p.id = engine.players.size();
            p.name = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].name : QString(getLocalizedText("Игрок %1", "Player %1")).arg(p.id + 1);
            p.avatar = (p.id < netManager->lobbyClients.size()) ? netManager->lobbyClients[p.id].avatar : 0;
            p.isBot = false;
            engine.players.append(p);
        }
    }
    QString act = json["act"].toString();
    if (act == "PLAY") {
        UnoColor col = static_cast<UnoColor>(json["chosenColor"].toInt(0));
        bool callUno = json["callUno"].toBool(false);
        engine.playCard(senderId, json["cardIdx"].toInt(), col, callUno);
    } else if (act == "DRAW") {
        engine.drawCard(senderId);
    } else if (act == "PASS") {
        engine.passTurn(senderId);
    } else if (act == "UNO") {
        engine.declareUno(senderId);
    } else if (act == "CATCH") {
        engine.catchUno(senderId, json["targetIdx"].toInt());
    }
    broadcastNetState();
}

void UnoWidget::broadcastNetState() {
    if (!netManager || !netManager->isHost) return;
    for (int i = 0; i < netManager->clientSockets.size(); ++i) {
        auto* socket = netManager->clientSockets[i];
        int targetPlayerId = i + 1;
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
    qreal s = getScale();

    int btnW = qRound(140 * s);
    int btnH = qRound(44 * s);
    int unoW = qRound(130 * s);
    int btnY = height() - btnH - qRound(18 * s);

    btnDrawCard->setGeometry(width() - btnW - qRound(20 * s), btnY, btnW, btnH);
    btnPass->setGeometry(width() - btnW - qRound(20 * s), btnY, btnW, btnH);
    btnUno->setGeometry(width() - btnW - unoW - qRound(30 * s), btnY, unoW, btnH);
    btnCatchUno->setGeometry(width() - btnW - unoW - qRound(30 * s), btnY, unoW, btnH);

    QFont btnFont(font().family(), qMax(8, qRound(12 * s)), QFont::Bold);
    btnDrawCard->setFont(btnFont);
    btnPass->setFont(btnFont);
    btnUno->setFont(btnFont);
    btnCatchUno->setFont(btnFont);

    int cpW = qRound(240 * s);
    int cpH = qRound(46 * s);
    colorPickerWidget->setGeometry(width() / 2 - cpW / 2, height() - cpH - qRound(55 * s), cpW, cpH);
    colorPickerWidget->setStyleSheet(QString(
        "background: rgba(15, 23, 42, 0.95); "
        "border-radius: %1px; "
        "border: 2px solid rgba(251, 191, 36, 0.6);"
    ).arg(cpH / 2));

    int circleSize = qRound(32 * s);
    int circleRadius = circleSize / 2;

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
            // Кнопка отмены "X"
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
        switch (AppSettings::instance().cardShirt) {
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
        QRectF backOval(-rect.width() * 0.33, -rect.height() * 0.49, rect.width() * 0.74, rect.height() * 1.02);
        p.drawEllipse(backOval);
        p.restore();

        p.save();
        p.translate(rect.center());
        p.rotate(-14);

        int backUnoFont = qMax(8, qRound(rect.height() * 0.16));
        QFont unoFont(p.font().family(), backUnoFont, QFont::Black);
        unoFont.setItalic(true);
        p.setFont(unoFont);

        QRectF textRect(-rect.width() / 2.0, -rect.height() / 2.0, rect.width(), rect.height());
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

    QRectF ovalRect(-rect.width() * 0.33, -rect.height() * 0.49, rect.width() * 0.74, rect.height() * 1.02);

    // 1. Отрисовка центрального овала
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

    // 2. Центральный символ (пропорционально масштабируемый шрифт)
    if (card->color == UnoWild) {
        if (card->value == UnoWildDrawFour) {
            int wild4Font = qMax(12, qRound(rect.height() * 0.26));
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

        int centerFontSize = qMax(10, qRound(rect.height() * (centerTxt.length() > 1 ? 0.22 : 0.30)));
        QFont centerFont(p.font().family(), centerFontSize, QFont::Black);
        centerFont.setItalic(true);
        p.setFont(centerFont);

        p.setPen(QColor(0, 0, 0, 80));
        p.drawText(rect.translated(2, 2), Qt::AlignCenter, centerTxt);

        p.setPen(Qt::white);
        p.drawText(rect, Qt::AlignCenter, centerTxt);
    }

    p.restore(); // Сброс clipPath

    // 3. Угловые индексы (пропорционально масштабируемые)
    int cornerFontSize = qMax(7, qRound(rect.height() * 0.11));

    if (card->color == UnoWild && card->value == UnoWildCard) {
        auto drawMiniWildOval = [&](const QPointF& pt) {
            qreal mw = rect.width() * 0.11;
            qreal mh = rect.height() * 0.13;
            QRectF miniRect(-mw / 2.0, -mh / 2.0, mw, mh);
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
        QRectF localRect(-rect.width() / 2.0, -rect.height() / 2.0, rect.width(), rect.height());
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
    auto& myHand = engine.players[engine.myIdx].hand;
    qreal s = getScale();

    int cardW = qRound(80 * s);
    int cardH = qRound(115 * s);
    int handY = height() - cardH - qRound(110 * s);
    int stepX = qMin(qRound(50 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    int newHovered = -1;
    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = (i == selectedHandCardIdx) ? qRound(-25 * s) : ((i == hoveredHandCardIdx) ? qRound(-12 * s) : 0);
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

    auto& myHand = engine.players[engine.myIdx].hand;
    qreal s = getScale();

    int cardW = qRound(80 * s);
    int cardH = qRound(115 * s);
    int handY = height() - cardH - qRound(110 * s);
    int stepX = qMin(qRound(50 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    bool clickedOnCard = false;

    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = (i == selectedHandCardIdx) ? qRound(-25 * s) : ((i == hoveredHandCardIdx) ? qRound(-12 * s) : 0);
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
            bool callUno = declaredUnoThisTurn;
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

    qreal s = getScale();

    if (!engine.players.isEmpty()) {
        int cardW = qRound(80 * s);
        int cardH = qRound(115 * s);

        int deckX = getSafeLeftMargin() + qRound(15 * s);
        int deckY = qRound(100 * s);
        drawDeckRect = QRect(deckX, deckY, cardW, cardH);

        if (engine.deck.size() > 1) drawUnoCard(p, QRect(deckX + 4, deckY + 4, cardW, cardH), nullptr, false);
        if (engine.deck.size() > 5) drawUnoCard(p, QRect(deckX + 2, deckY + 2, cardW, cardH), nullptr, false);
        drawUnoCard(p, drawDeckRect, nullptr, false);

        QRect badgeRect(deckX - 5, deckY + cardH + qRound(8 * s), cardW + 10, qRound(22 * s));
        p.setBrush(QColor(15, 23, 42, 220));
        p.setPen(QPen(QColor(251, 191, 36, 180), 1));
        p.drawRoundedRect(badgeRect, 6, 6);
        p.setFont(QFont(font().family(), qMax(8, qRound(10 * s)), QFont::Bold));
        p.setPen(Qt::white);
        p.drawText(badgeRect, Qt::AlignCenter, QString(getLocalizedText("Карт: %1", "Cards: %1")).arg(engine.deck.size()));

        drawCenterDiscard(p, cardW, cardH);
        drawPlayers(p, cardW, cardH);

        if (engine.myIdx < engine.players.size()) {
            auto& myHand = engine.players[engine.myIdx].hand;
            int handY = height() - cardH - qRound(110 * s);
            int stepHandX = qMin(qRound(50 * s), (width() - qRound(300 * s)) / qMax<int>(1, myHand.size()));
            int startX = (width() - (myHand.size() * stepHandX + (cardW - stepHandX))) / 2;
            for (int i = 0; i < myHand.size(); ++i) {
                bool isSelected = (i == selectedHandCardIdx);
                bool isHovered  = (i == hoveredHandCardIdx);
                int offsetY     = isSelected ? qRound(-25 * s) : (isHovered ? qRound(-12 * s) : 0);
                drawUnoCard(p, QRect(startX + i * stepHandX, handY + offsetY, cardW, cardH), &myHand[i], true, isSelected);
            }
        }
    }

    if (engine.gameOver) {
        drawGameOverBanner(p, engine.statusMessage);
    }
}

void UnoWidget::drawCenterDiscard(QPainter& p, int cardW, int cardH) {
    qreal s = getScale();
    QPoint center(width() / 2, height() / 2 - qRound(25 * s));

    const QColor arrowColors[] = { QColor(220, 38, 38), QColor(234, 179, 8), QColor(22, 163, 74), QColor(37, 99, 235) };
    QColor curCol = arrowColors[engine.currentColor];

    int arrowRadius = qRound(105 * s);
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
        qreal aS = s * 9.0;
        if (engine.direction == 1) arrow << QPointF(-aS, -aS * 0.8) << QPointF(aS, 0) << QPointF(-aS, aS * 0.8);
        else                       arrow << QPointF(aS, -aS * 0.8) << QPointF(-aS, 0) << QPointF(aS, aS * 0.8);
        p.drawPolygon(arrow);
        p.restore();
    }
    p.restore();

    int discardX = center.x() - cardW / 2;
    int discardY = center.y() - cardH / 2;

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
    int numPlayers = engine.players.size();
    qreal s = getScale();
    QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), qRound(75 * s), qRound(80 * s));

    int boxW = qRound(150 * s);
    int boxH = qRound(42 * s);

    for (int i = 0; i < numPlayers; ++i) {
        int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        QPoint pos = seatPos[displayIdx];
        auto& opp = engine.players[i];
        int handSize = opp.hand.size();

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
        QString nameWithAvatar = getAvatarEmojiById(opp.avatar) + " " + opp.name;
        p.drawText(QRect(pos.x() - boxW / 2 + 5, pos.y() - boxH / 2, boxW - 10, boxH), Qt::AlignVCenter | Qt::AlignLeft, nameWithAvatar);

        int badgeSize = qRound(38 * s);
        QRect badgeRect(pos.x() + boxW / 2 + qRound(5 * s), pos.y() - badgeSize / 2, badgeSize, badgeSize);
        bool isUno = (handSize == 1);
        p.setBrush(isUno ? QColor(220, 38, 38) : QColor(30, 41, 59, 240));
        p.setPen(QPen(isUno ? QColor(254, 240, 138) : QColor(255, 255, 255, 60), isUno ? 2 : 1));
        p.drawRoundedRect(badgeRect, 6, 6);

        p.setFont(QFont(font().family(), qMax(8, qRound((isUno ? 9 : 11) * s)), QFont::Bold));
        p.setPen(Qt::white);
        p.drawText(badgeRect, Qt::AlignCenter, isUno ? "UNO!" : QString("x%1").arg(handSize));

        if (displayIdx != 0) {
            int oppStep = qRound(15 * s);
            int oppW = cardW - qRound(25 * s);
            int oppH = cardH - qRound(35 * s);
            int startX = pos.x() - (handSize * oppStep + (oppW - oppStep)) / 2;
            for (int c = 0; c < handSize; ++c) {
                drawUnoCard(p, QRect(startX + c * oppStep, pos.y() + boxH / 2 + qRound(5 * s), oppW, oppH), nullptr, false);
            }
        }
    }
}

// ============================================================================
// MAIN WINDOW
// ============================================================================

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(getLocalizedText("Royal Card Club Collection", "Royal Card Club Collection"));
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    resize(1280, 720);
    setMinimumSize(640, 360);

    auto* f11Shortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(f11Shortcut, &QShortcut::activated, this, [this]() {
        if (isFullScreen()) {
            showNormal();
            AppSettings::instance().fullScreen = false;
        } else {
            showFullScreen();
            AppSettings::instance().fullScreen = true;
        }
    });
#endif

    netManager = new NetworkManager(this);

    stackedWidget = new QStackedWidget(this);
    menuWidget    = new MainMenuWidget(this);
    pokerWidget   = new PokerWidget(netManager, this);
    durakWidget   = new DurakWidget(netManager, this);
    kozelWidget   = new KozelWidget(netManager, this);
    unoWidget     = new UnoWidget(netManager, this);

    stackedWidget->addWidget(menuWidget);
    stackedWidget->addWidget(pokerWidget);
    stackedWidget->addWidget(durakWidget);
    stackedWidget->addWidget(kozelWidget);
    stackedWidget->addWidget(unoWidget);
    // Фикс QComboBox для Android (QTBUG-127495)
#if defined(Q_OS_ANDROID)
    auto* glContainer = new QOpenGLWidget(this);
    auto* glLayout = new QVBoxLayout(glContainer);
    glLayout->setContentsMargins(0, 0, 0, 0);
    glLayout->addWidget(stackedWidget);
    setCentralWidget(glContainer);
#else
    setCentralWidget(stackedWidget);
#endif

    connect(netManager, &NetworkManager::signalStartNetworkGame, this, [this](int gType, int clients) {
        if (gType == 1) {
            durakWidget->engine.initGame(clients, true);
            for (int i = 0; i < netManager->lobbyClients.size() && i < durakWidget->engine.players.size(); ++i) {
                durakWidget->engine.players[i].name   = netManager->lobbyClients[i].name;
                durakWidget->engine.players[i].avatar = netManager->lobbyClients[i].avatar;
            }
            durakWidget->broadcastNetState();
            stackedWidget->setCurrentIndex(2);
        } else if (gType == 2) {
            kozelWidget->engine.initGame(clients, true);
            for (int i = 0; i < netManager->lobbyClients.size() && i < kozelWidget->engine.players.size(); ++i) {
                kozelWidget->engine.players[i].name   = netManager->lobbyClients[i].name;
                kozelWidget->engine.players[i].avatar = netManager->lobbyClients[i].avatar;
            }
            kozelWidget->broadcastNetState();
            stackedWidget->setCurrentIndex(3);
        } else if (gType == 3) {
            unoWidget->engine.initGame(clients, true);
            for (int i = 0; i < netManager->lobbyClients.size() && i < unoWidget->engine.players.size(); ++i) {
                unoWidget->engine.players[i].name   = netManager->lobbyClients[i].name;
                unoWidget->engine.players[i].avatar = netManager->lobbyClients[i].avatar;
            }
            unoWidget->broadcastNetState();
            stackedWidget->setCurrentIndex(4);
        }
    });

    connect(netManager, &NetworkManager::signalClientGameStarted, this, [this](int gType, const QJsonObject& json) {
        if (stackedWidget->currentIndex() == 0) return;

        if (gType == 0) {
            pokerWidget->engine.fromJson(json);
            stackedWidget->setCurrentIndex(1);
        } else if (gType == 1) {
            durakWidget->engine.fromJson(json);
            stackedWidget->setCurrentIndex(2);
        } else if (gType == 2) {
            kozelWidget->engine.fromJson(json);
            stackedWidget->setCurrentIndex(3);
        } else if (gType == 3) {
            unoWidget->engine.fromJson(json);
            stackedWidget->setCurrentIndex(4);
        }
    });

    connect(menuWidget->btnStartBotGame, &QPushButton::clicked, this, [this]() {
        int gameType = menuWidget->comboGameType->currentData().toInt();
        int botCount = menuWidget->comboBots->currentData().toInt();

        if (gameType == 0) {
            pokerWidget->startSingleGame(botCount);
            stackedWidget->setCurrentIndex(1);
        } else if (gameType == 1) {
            durakWidget->startSingleGame(botCount);
            stackedWidget->setCurrentIndex(2);
        } else if (gameType == 2) {
            kozelWidget->startSingleGame(botCount);
            stackedWidget->setCurrentIndex(3);
        } else if (gameType == 3) {
            unoWidget->startSingleGame(botCount);
            stackedWidget->setCurrentIndex(4);
        }
    });

    connect(menuWidget->btnHostServer, &QPushButton::clicked, this, [this]() {
        // 1. Останавливаем таймеры ботов
        pokerWidget->aiTimer->stop();
        durakWidget->aiTimer->stop();
        kozelWidget->aiTimer->stop();
        unoWidget->aiTimer->stop();

        // 2. Сбрасываем старые данные одиночных игр
        pokerWidget->engine.players.clear();
        pokerWidget->engine.communityCards.clear();
        pokerWidget->engine.pot = 0;
        pokerWidget->engine.gameOver = false;
        pokerWidget->engine.statusMessage.clear();

        durakWidget->engine.players.clear();
        durakWidget->engine.table.clear();
        durakWidget->engine.gameOver = false;

        kozelWidget->engine.players.clear();
        kozelWidget->engine.currentTrick.clear();
        kozelWidget->engine.gameOver = false;

        unoWidget->engine.players.clear();
        unoWidget->engine.discardPile.clear();
        unoWidget->engine.gameOver = false;

        // 3. Запускаем сервер
        int gameType = menuWidget->comboGameType->currentData().toInt();
        netManager->startHostServer(gameType);

        pokerWidget->updateUI();
        stackedWidget->setCurrentIndex(1);
    });

    connect(menuWidget->btnConnectIP, &QPushButton::clicked, this, [this]() {
        // 1. Останавливаем таймеры ботов
        pokerWidget->aiTimer->stop();
        durakWidget->aiTimer->stop();
        kozelWidget->aiTimer->stop();

        // 2. Сбрасываем старые данные одиночных игр
        pokerWidget->engine.players.clear();
        pokerWidget->engine.communityCards.clear();
        pokerWidget->engine.pot = 0;
        pokerWidget->engine.gameOver = false;
        pokerWidget->engine.statusMessage.clear();

        durakWidget->engine.players.clear();
        durakWidget->engine.table.clear();
        durakWidget->engine.gameOver = false;

        kozelWidget->engine.players.clear();
        kozelWidget->engine.currentTrick.clear();
        kozelWidget->engine.gameOver = false;

        // 3. Подключаемся к серверу
        int gameType = menuWidget->comboGameType->currentData().toInt();
        netManager->connectToHost(menuWidget->ipInput->text(), gameType);

        pokerWidget->updateUI();
        stackedWidget->setCurrentIndex(1);
    });

    connect(netManager, &NetworkManager::signalPlayerReconnected, this, [this](int pIdx) {
        if (netManager->isHost) {
            if (netManager->gameType == 0) {
                pokerWidget->engine.handlePlayerReconnect(pIdx);
                pokerWidget->broadcastNetState();
            } else if (netManager->gameType == 1) {
                durakWidget->engine.handlePlayerReconnect(pIdx);
                durakWidget->broadcastNetState();
            } else if (netManager->gameType == 2) {
                kozelWidget->engine.handlePlayerReconnect(pIdx);
                kozelWidget->broadcastNetState();
            } else if (netManager->gameType == 3) {
                unoWidget->engine.handlePlayerReconnect(pIdx);
                unoWidget->broadcastNetState();
            }
        }
    });

    connect(netManager, &NetworkManager::signalHostDisconnected, this, [this]() {
        netManager->isLobby = false;
        pokerWidget->engine.gameOver = true;
        pokerWidget->engine.statusMessage = getLocalizedText("Связь с сервером потеряна! Хост отключился.", "Connection lost! Host disconnected.");

        durakWidget->engine.gameOver = true;
        durakWidget->engine.statusMessage = getLocalizedText("Связь с сервером потеряна! Хост отключился.", "Connection lost! Host disconnected.");

        kozelWidget->engine.gameOver = true;
        kozelWidget->engine.statusMessage = getLocalizedText("Связь с сервером потеряна! Хост отключился.", "Connection lost! Host disconnected.");

        unoWidget->engine.gameOver = true;
        unoWidget->engine.statusMessage = getLocalizedText("Связь с сервером потеряна! Хост отключился.", "Connection lost! Host disconnected.");

        if (stackedWidget->currentIndex() == 1) pokerWidget->updateUI();
        else if (stackedWidget->currentIndex() == 2) durakWidget->updateUI();
        else if (stackedWidget->currentIndex() == 3) kozelWidget->updateUI();
        else if (stackedWidget->currentIndex() == 4) unoWidget->updateUI();
    });

        connect(netManager, &NetworkManager::signalNetworkDataReceived, this, [this](int senderId, const QJsonObject& json) {
            if (!netManager->isHost && stackedWidget->currentIndex() == 0) return;

            if (netManager->isHost) {
                QString act = json["act"].toString();

                // Обработка смены имени/аватара клиентом во время игры
                if (act == "UPDATE_PROFILE") {
                    QString newName = json["name"].toString().trimmed();
                    int newAvatar   = json["avatar"].toInt(0);
                    if (newName.isEmpty()) newName = getLocalizedText("Игрок", "Player");

                    if (senderId < netManager->lobbyClients.size()) {
                        netManager->lobbyClients[senderId].name   = newName;
                        netManager->lobbyClients[senderId].avatar = newAvatar;
                    }

                    auto updatePlayerInEngine = [&](auto& engine) {
                        if (senderId < engine.players.size()) {
                            engine.players[senderId].name   = newName;
                            engine.players[senderId].avatar = newAvatar;
                        }
                    };

                    if (netManager->gameType == 0) {
                        updatePlayerInEngine(pokerWidget->engine);
                        pokerWidget->broadcastNetState();
                        pokerWidget->updateUI();
                    } else if (netManager->gameType == 1) {
                        updatePlayerInEngine(durakWidget->engine);
                        durakWidget->broadcastNetState();
                        durakWidget->updateUI();
                    } else if (netManager->gameType == 2) {
                        updatePlayerInEngine(kozelWidget->engine);
                        kozelWidget->broadcastNetState();
                        kozelWidget->updateUI();
                    } else if (netManager->gameType == 3) {
                        updatePlayerInEngine(unoWidget->engine);
                        unoWidget->broadcastNetState();
                        unoWidget->updateUI();
                    }
                    return;
                }

                if (netManager->gameType == 0) {
                    pokerWidget->processNetAction(senderId, json);
                } else if (netManager->gameType == 1) {
                    durakWidget->processNetAction(senderId, json);
                } else if (netManager->gameType == 2) {
                    kozelWidget->processNetAction(senderId, json);
                } else if (netManager->gameType == 3) {
                    unoWidget->processNetAction(senderId, json);
                }
            } else {
                int gType = json["gameType"].toInt();
                if (gType == 0) pokerWidget->engine.fromJson(json);
                else if (gType == 1) durakWidget->engine.fromJson(json);
                else if (gType == 2) kozelWidget->engine.fromJson(json);
                else if (gType == 3) unoWidget->engine.fromJson(json);
            }
        });

        connect(netManager, &NetworkManager::signalPlayerDisconnected, this, [this](int pIdx) {
            if (netManager->isHost) {
                if (netManager->gameType == 0) {
                    pokerWidget->engine.handlePlayerDisconnect(pIdx);
                    pokerWidget->broadcastNetState();
                } else if (netManager->gameType == 1) {
                    durakWidget->engine.handlePlayerDisconnect(pIdx);
                    durakWidget->broadcastNetState();
                } else if (netManager->gameType == 2) {
                    kozelWidget->engine.handlePlayerDisconnect(pIdx);
                    kozelWidget->broadcastNetState();
                } else if (netManager->gameType == 3) {
                    unoWidget->engine.handlePlayerDisconnect(pIdx);
                    unoWidget->broadcastNetState();
                }
            }
        });

        auto returnToLobby = [this]() {
            netManager->isLobby = true;
            pokerWidget->engine.gameOver = false;
            pokerWidget->engine.players.resize(1);
            pokerWidget->engine.players[0].id = 0;
            pokerWidget->engine.players[0].name = AppSettings::instance().nickname;
            pokerWidget->engine.players[0].avatar = static_cast<int>(AppSettings::instance().avatar);
            pokerWidget->engine.players[0].holeCards.clear();
            pokerWidget->engine.players[0].currentBet = 0;
            pokerWidget->engine.players[0].hasFolded = false;

            for (auto* s : netManager->clientSockets) {
                if (s) {
                    s->abort();
                    s->disconnect();
                    s->deleteLater();
                }
            }
            netManager->clientSockets.clear();

            durakWidget->engine.gameOver = false;
            durakWidget->engine.players.clear();
            durakWidget->engine.table.clear();

            kozelWidget->engine.gameOver = false;
            kozelWidget->engine.players.clear();
            kozelWidget->engine.currentTrick.clear();

            static const QString gameNames[] = { getLocalizedText("ПОКЕРА", "POKER"), getLocalizedText("ДУРАКА", "DURAK"), getLocalizedText("КОЗЛА", "KOZEL"), getLocalizedText("УНО", "UNO") };
            QString statusMsg = QString(getLocalizedText("ЛОББИ (%1): 1/%2 игроков. Ожидание...", "LOBBY (%1): 1/%2 players. Waiting..."))
            .arg(gameNames[netManager->gameType])
            .arg(NetConfig::MAX_PLAYERS);

            QJsonObject lobbyJson;
            lobbyJson["isLobby"]     = true;
            lobbyJson["gameType"]    = netManager->gameType;
            lobbyJson["playerCount"] = 1;
            netManager->broadcastJson(lobbyJson);

            pokerWidget->lblStatus->setText(statusMsg);
            stackedWidget->setCurrentIndex(1);
            pokerWidget->updateUI();
        };

        pokerWidget->onReturnToLobbyCallback = returnToLobby;
        durakWidget->onReturnToLobbyCallback = returnToLobby;
        kozelWidget->onReturnToLobbyCallback = returnToLobby;
        unoWidget->onReturnToLobbyCallback   = returnToLobby;

        auto returnToMenu = [this]() {
            pokerWidget->aiTimer->stop();
            pokerWidget->autoNextHandTimer->stop();
            durakWidget->aiTimer->stop();
            kozelWidget->aiTimer->stop();
            unoWidget->aiTimer->stop();

            netManager->disconnectAll();

            pokerWidget->engine.gameOver = true;
            durakWidget->engine.gameOver = true;
            durakWidget->engine.players.clear();
            durakWidget->engine.table.clear();
            durakWidget->engine.deck.clear();

            kozelWidget->engine.gameOver = true;
            kozelWidget->engine.players.clear();
            kozelWidget->engine.currentTrick.clear();
            kozelWidget->engine.deck.clear();

            unoWidget->engine.gameOver = true;
            unoWidget->engine.players.clear();
            unoWidget->engine.discardPile.clear();
            unoWidget->engine.deck.clear();

            stackedWidget->setCurrentIndex(0);
        };

        pokerWidget->onBackToMenuCallback = returnToMenu;
        durakWidget->onBackToMenuCallback = returnToMenu;
        kozelWidget->onBackToMenuCallback = returnToMenu;
        unoWidget->onBackToMenuCallback   = returnToMenu;

        // Логика мгновенного применения настроек во время игры
        auto applySettingsChanges = [this]() {
            // 1. Обновляем имя и аватар локального игрока во всех движках
            auto updateLocalPlayer = [](auto& engine) {
                if (engine.myIdx < engine.players.size()) {
                    engine.players[engine.myIdx].name   = AppSettings::instance().nickname;
                    engine.players[engine.myIdx].avatar = static_cast<int>(AppSettings::instance().avatar);
                }
            };
            updateLocalPlayer(pokerWidget->engine);
            updateLocalPlayer(durakWidget->engine);
            updateLocalPlayer(kozelWidget->engine);
            updateLocalPlayer(unoWidget->engine);

            // 2. Если это Хост или одиночная игра — мгновенно применяем новые правила Уно
            unoWidget->engine.drawMode        = AppSettings::instance().unoDrawMode;
            unoWidget->engine.stackingEnabled = AppSettings::instance().unoStacking;

            // 3. Сетевая синхронизация изменений
            if (netManager && netManager->isNetworkGame) {
                if (netManager->isHost) {
                    if (!netManager->lobbyClients.isEmpty()) {
                        netManager->lobbyClients[0].name   = AppSettings::instance().nickname;
                        netManager->lobbyClients[0].avatar = static_cast<int>(AppSettings::instance().avatar);
                    }
                    // Хост сразу рассылает новое состояние (новые имена, аватарки, правила) всем клиентам
                    if (netManager->gameType == 0)      pokerWidget->broadcastNetState();
                    else if (netManager->gameType == 1) durakWidget->broadcastNetState();
                    else if (netManager->gameType == 2) kozelWidget->broadcastNetState();
                    else if (netManager->gameType == 3) unoWidget->broadcastNetState();
                } else {
                    // Клиент отправляет хосту свои новые данные профиля
                    QJsonObject json;
                    json["act"]    = "UPDATE_PROFILE";
                    json["name"]   = AppSettings::instance().nickname;
                    json["avatar"] = static_cast<int>(AppSettings::instance().avatar);
                    netManager->sendJsonToServer(json);
                }
            }

            // 4. Мгновенно перерисовываем стол (новое сукно, рубашки, аватары)
            pokerWidget->updateUI();
            durakWidget->updateUI();
            kozelWidget->updateUI();
            unoWidget->updateUI();
        };

        auto openInGameSettings = [this, applySettingsChanges]() {
            SettingsDialog dialog(this);
            if (dialog.exec() == QDialog::Accepted) {
                applySettingsChanges();
            }
        };

        // Привязываем открытие настроек ко всем игровым столам
        pokerWidget->onOpenSettingsCallback = openInGameSettings;
        durakWidget->onOpenSettingsCallback = openInGameSettings;
        kozelWidget->onOpenSettingsCallback = openInGameSettings;
        unoWidget->onOpenSettingsCallback   = openInGameSettings;

        connect(menuWidget->btnRules, &QPushButton::clicked, this, [this]() {
            AudioManager::instance().playSound(SoundEffect::ButtonClick);
            int curGame = menuWidget->comboGameType->currentData().toInt();
            RulesDialog dialog(curGame, this);
            dialog.exec();
        });

        pokerWidget->onOpenRulesCallback = [this]() { RulesDialog(0, this).exec(); };
        durakWidget->onOpenRulesCallback = [this]() { RulesDialog(1, this).exec(); };
        kozelWidget->onOpenRulesCallback = [this]() { RulesDialog(2, this).exec(); };
        unoWidget->onOpenRulesCallback   = [this]() { RulesDialog(3, this).exec(); };
}

MainWindow::~MainWindow() {
    delete netManager;
}
