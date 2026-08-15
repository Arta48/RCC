#include <QTabWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextBrowser>

#include "AppSettings.h"
#include "MainWindow.h"
#include "Audio.h"

// ============================================================================
// НАСТРОЙКИ (ДИАЛОГОВОЕ ОКНО)
// ============================================================================

class SettingsDialog : public QDialog {
public:
    explicit SettingsDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(getLocalizedText("Настройки", "Settings"));
        setFixedSize(500, 440);
        setStyleSheet(R"(
            QDialog {
                background-color: #0B1120;
                border: 1px solid rgba(251, 191, 36, 0.35);
                border-radius: 16px;
            }
            QLabel {
                color: #E2E8F0;
                font-size: 13px;
                font-weight: bold;
            }
            QTabWidget::pane {
                border: 1px solid rgba(251, 191, 36, 0.2);
                border-radius: 12px;
                background: rgba(15, 23, 42, 0.85);
            }
            QTabBar::tab {
                background: transparent;
                color: #94A3B8;
                padding: 10px 20px;
                font-size: 13px;
                font-weight: bold;
                border-bottom: 2px solid transparent;
            }
            QTabBar::tab:hover {
                color: #F8FAFC;
            }
            QTabBar::tab:selected {
                color: #FBBF24;
                border-bottom: 2px solid #FBBF24;
                background: rgba(251, 191, 36, 0.08);
                border-top-left-radius: 8px;
                border-top-right-radius: 8px;
            }
            QComboBox, QLineEdit, QSpinBox {
                padding: 8px 12px;
                border-radius: 8px;
                background: #1E293B;
                color: #F8FAFC;
                border: 1px solid #334155;
                font-size: 13px;
            }
            QComboBox:hover, QLineEdit:focus, QSpinBox:focus {
                border: 1px solid #F59E0B;
            }
            QComboBox::drop-down {
                border: none;
                width: 24px;
            }
            QCheckBox {
                color: #E2E8F0;
                font-size: 13px;
                font-weight: bold;
                spacing: 10px;
            }
            QSlider::groove:horizontal {
                height: 6px;
                background: #0F172A;
                border-radius: 3px;
            }
            QSlider::sub-page:horizontal {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #D97706, stop:1 #FBBF24);
                border-radius: 3px;
            }
            QSlider::handle:horizontal {
                background: #FBBF24;
                width: 18px;
                height: 18px;
                margin-top: -6px;
                margin-bottom: -6px;
                border-radius: 9px;
                border: 2px solid #78350F;
            }
        )");

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);

        auto* title = new QLabel(getLocalizedText("⚙ НАСТРОЙКИ", "⚙ SETTINGS"), this);
        title->setStyleSheet("color: #FBBF24; font-size: 20px; font-weight: 900; letter-spacing: 2px;");
        title->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(title);

        auto* tabs = new QTabWidget(this);

        // ВКЛАДКА 1: ЗВУК
        auto* tabAudio = new QWidget(this);
        auto* aLayout = new QVBoxLayout(tabAudio);
        aLayout->setContentsMargins(24, 25, 24, 25);
        aLayout->setSpacing(25);

        auto* mBox = new QHBoxLayout();
        auto* lblMusic = new QLabel(getLocalizedText("🎵 Музыка:", "🎵 Music:"), tabAudio);
        auto* sMusic = new QSlider(Qt::Horizontal, tabAudio);
        sMusic->setRange(0, 100);
        sMusic->setSingleStep(5);
        sMusic->setPageStep(5);

        int curMusicVal = qRound(AudioManager::instance().getMusicVolume() * 100.0f);
        curMusicVal = (curMusicVal / 5) * 5;
        sMusic->setValue(curMusicVal);

        auto* lblMusicVal = new QLabel(QString("%1%").arg(curMusicVal), tabAudio);
        lblMusicVal->setFixedWidth(45);
        lblMusicVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lblMusicVal->setStyleSheet("color: #FCD34D; font-size: 14px; font-weight: bold;");

        connect(sMusic, &QSlider::valueChanged, this, [lblMusicVal, sMusic](int val) {
            int snapped = (val / 5) * 5;
            if (snapped != val) {
                sMusic->setValue(snapped);
                return;
            }
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

        int curSfxVal = qRound(AudioManager::instance().getSfxVolume() * 100.0f);
        curSfxVal = (curSfxVal / 5) * 5;
        sSfx->setValue(curSfxVal);

        auto* lblSfxVal = new QLabel(QString("%1%").arg(curSfxVal), tabAudio);
        lblSfxVal->setFixedWidth(45);
        lblSfxVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lblSfxVal->setStyleSheet("color: #FCD34D; font-size: 14px; font-weight: bold;");

        connect(sSfx, &QSlider::valueChanged, this, [lblSfxVal, sSfx](int val) {
            int snapped = (val / 5) * 5;
            if (snapped != val) {
                sSfx->setValue(snapped);
                return;
            }
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
        auto* tabVisual = new QWidget(this);
        auto* vLayout = new QVBoxLayout(tabVisual);
        vLayout->setContentsMargins(24, 20, 24, 20);
        vLayout->setSpacing(12);

        auto* nickBox = new QHBoxLayout();
        nickBox->addWidget(new QLabel(getLocalizedText("Имя игрока:", "Player name:"), tabVisual));
        auto* nickInput = new QLineEdit(AppSettings::instance().nickname, tabVisual);
        nickBox->addWidget(nickInput);
        vLayout->addLayout(nickBox);

        auto* avBox = new QHBoxLayout();
        avBox->addWidget(new QLabel(getLocalizedText("Аватар:", "Avatar:"), tabVisual));
        auto* avCombo = new QComboBox(tabVisual);
        avCombo->addItem(getLocalizedText("👑 Корона", "👑 Crown"), static_cast<int>(AvatarIcon::Crown));
        avCombo->addItem(getLocalizedText("💀 Череп", "💀 Skull"), static_cast<int>(AvatarIcon::Skull));
        avCombo->addItem(getLocalizedText("♠ Масть Пики", "♠ Spade Suit"), static_cast<int>(AvatarIcon::SuitSpade));
        avCombo->addItem(getLocalizedText("🃏 Джокер", "🃏 Joker"), static_cast<int>(AvatarIcon::Joker));
        avCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().avatar));
        avBox->addWidget(avCombo);
        vLayout->addLayout(avBox);

        auto* colorBox = new QHBoxLayout();
        colorBox->addWidget(new QLabel(getLocalizedText("Цвет сукна:", "Table felt:"), tabVisual));
        auto* colorCombo = new QComboBox(tabVisual);
        colorCombo->addItem(getLocalizedText("🟢 Зелёный (Классика)", "🟢 Green (Classic)"), static_cast<int>(TableColor::ClassicGreen));
        colorCombo->addItem(getLocalizedText("🔴 Бордовый", "🔴 Burgundy"), static_cast<int>(TableColor::BurgundyRed));
        colorCombo->addItem(getLocalizedText("🔵 Тёмно-синий", "🔵 Dark Blue"), static_cast<int>(TableColor::DarkBlue));
        colorCombo->addItem(getLocalizedText("🖤 Покерный чёрный", "🖤 Poker Black"), static_cast<int>(TableColor::PokerBlack));
        colorCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().tableColor));
        colorBox->addWidget(colorCombo);
        vLayout->addLayout(colorBox);

        auto* shirtBox = new QHBoxLayout();
        shirtBox->addWidget(new QLabel(getLocalizedText("Рубашка карт:", "Card shirt:"), tabVisual));
        auto* shirtCombo = new QComboBox(tabVisual);
        shirtCombo->addItem(getLocalizedText("🟦 Классическая синяя", "🟦 Classic Blue"), static_cast<int>(CardShirtStyle::ClassicBlue));
        shirtCombo->addItem(getLocalizedText("🟥 Красный бархат", "🟥 Red Velvet"), static_cast<int>(CardShirtStyle::RedVelvet));
        shirtCombo->addItem(getLocalizedText("🟨 Золотая Royal", "🟨 Gold Royal"), static_cast<int>(CardShirtStyle::GoldRoyal));
        shirtCombo->addItem(getLocalizedText("⬛ Тёмная с узором", "⬛ Dark Pattern"), static_cast<int>(CardShirtStyle::DarkPattern));
        shirtCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().cardShirt));
        shirtBox->addWidget(shirtCombo);
        vLayout->addLayout(shirtBox);

        auto* chkFull = new QCheckBox(getLocalizedText("Полноэкранный режим (F11)", "Fullscreen Mode (F11)"), tabVisual);
        chkFull->setChecked(AppSettings::instance().fullScreen);
        vLayout->addWidget(chkFull);

        vLayout->addStretch();
        tabs->addTab(tabVisual, getLocalizedText("Визуал", "Visuals"));

        // ВКЛАДКА 3: ГЕЙМПЛЕЙ И СЕТЬ
        auto* tabGame = new QWidget(this);
        auto* gLayout = new QVBoxLayout(tabGame);
        gLayout->setContentsMargins(24, 25, 24, 25);
        gLayout->setSpacing(20);

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
        auto* unoDrawCombo = new QComboBox(tabGame);
        unoDrawCombo->addItem(getLocalizedText("Взять 1 карту и пас", "Draw 1 card and pass"), static_cast<int>(UnoDrawMode::DrawOne));
        unoDrawCombo->addItem(getLocalizedText("Тянуть до подходящей", "Draw until matching card"), static_cast<int>(UnoDrawMode::DrawUntilMatch));
        unoDrawCombo->setCurrentIndex(static_cast<int>(AppSettings::instance().unoDrawMode));
        unoDrawBox->addWidget(unoDrawCombo);
        gLayout->addLayout(unoDrawBox);

        auto* portBox = new QHBoxLayout();
        portBox->addWidget(new QLabel(getLocalizedText("Порт сервера:", "Server port:"), tabGame));
        auto* portSpin = new QSpinBox(tabGame);
        portSpin->setRange(1024, 65535);
        portSpin->setValue(AppSettings::instance().serverPort);
        portBox->addWidget(portSpin);
        gLayout->addLayout(portBox);

        gLayout->addStretch();
        tabs->addTab(tabGame, getLocalizedText("Геймплей и Сеть", "Gameplay && Network"));

        mainLayout->addWidget(tabs);

        auto* btnSave = new QPushButton(getLocalizedText("Сохранить и применить", "Save and apply"), this);
        btnSave->setCursor(Qt::PointingHandCursor);
        btnSave->setStyleSheet(R"(
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #059669, stop:1 #10B981);
                color: white;
                font-weight: bold;
                font-size: 15px;
                padding: 12px;
                border-radius: 8px;
                border: 1px solid rgba(52, 211, 153, 0.4);
            }
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #10B981, stop:1 #34D399);
            }
        )");

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

            if (parentWidget() && parentWidget()->window()) {
                if (AppSettings::instance().fullScreen) parentWidget()->window()->showFullScreen();
                else parentWidget()->window()->showNormal();
            }

            AppSettings::instance().save();
            accept();
        });
        mainLayout->addWidget(btnSave);
    }
};

// ============================================================================
// ПРАВИЛА (ДИАЛОГОВОЕ ОКНО)
// ============================================================================

class RulesDialog : public QDialog {
public:
    explicit RulesDialog(int defaultTabIndex = 0, QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(getLocalizedText("Правила игры", "Game Rules"));
        setFixedSize(680, 540);
        setStyleSheet(R"(
            QDialog {
                background-color: #0B1120;
                border: 1px solid rgba(251, 191, 36, 0.35);
                border-radius: 16px;
            }
            QLabel { color: #FBBF24; font-size: 20px; font-weight: 900; }
            QTabWidget::pane {
                border: 1px solid rgba(251, 191, 36, 0.2);
                border-radius: 12px;
                background: rgba(15, 23, 42, 0.9);
            }
            QTabBar::tab {
                background: transparent;
                color: #94A3B8;
                padding: 10px 18px;
                font-size: 13px;
                font-weight: bold;
                border-bottom: 2px solid transparent;
            }
            QTabBar::tab:hover { color: #F8FAFC; }
            QTabBar::tab:selected {
                color: #FBBF24;
                border-bottom: 2px solid #FBBF24;
                background: rgba(251, 191, 36, 0.08);
                border-top-left-radius: 8px;
                border-top-right-radius: 8px;
            }
            QTextBrowser {
                background: transparent;
                border: none;
                color: #E2E8F0;
                font-size: 13px;
                line-height: 1.5;
                padding: 12px;
            }
            QScrollBar:vertical {
                background: #0F172A;
                width: 8px;
                border-radius: 4px;
            }
            QScrollBar::handle:vertical {
                background: #334155;
                border-radius: 4px;
            }
        )");

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 18, 20, 18);
        mainLayout->setSpacing(12);

        auto* title = new QLabel(getLocalizedText("📖 ПРАВИЛА ИГР", "📖 GAME RULES"), this);
        title->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(title);

        auto* tabs = new QTabWidget(this);

        auto createRuleTab = [&](const QString& textHtml) -> QWidget* {
            auto* page = new QWidget(this);
            auto* l = new QVBoxLayout(page);
            l->setContentsMargins(10, 10, 10, 10);
            auto* browser = new QTextBrowser(page);
            browser->setHtml(textHtml);
            l->addWidget(browser);
            return page;
        };

        // ТЕКСТЫ ПРАВИЛ
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
        btnClose->setStyleSheet(R"(
            QPushButton {
                background: rgba(30, 41, 59, 0.9);
                color: #F8FAFC;
                font-weight: bold;
                font-size: 14px;
                padding: 10px;
                border-radius: 8px;
                border: 1px solid #475569;
            }
            QPushButton:hover { background: #334155; border: 1px solid #FBBF24; }
        )");
        connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
        mainLayout->addWidget(btnClose);
    }
};

// ============================================================================
// BASE TABLE WIDGET
// ============================================================================

BaseTableWidget::BaseTableWidget(QWidget* parent) : QWidget(parent) {
#if !defined(Q_OS_IOS)
    setMinimumSize(1000, 720);
#endif
    setMouseTracking(true);

    btnBackMenu = new QPushButton(getLocalizedText("← В Меню", "← Menu"), this);
    lblStatus   = new QLabel("", this);
    btnNextHand = new QPushButton(getLocalizedText("ИГРАТЬ ЗАНОВО", "PLAY AGAIN"), this);
    btnSettings = new QPushButton("⚙", this);
    btnRules    = new QPushButton("📖", this);

    btnBackMenu->setStyleSheet("QPushButton { background: rgba(0,0,0,0.5); color: #D1D5DB; border-radius: 6px; padding: 5px; font-weight: bold; } QPushButton:hover { background: rgba(0,0,0,0.7); color: white; }");
    lblStatus->setStyleSheet("QLabel { color: #F3F4F6; font-size: 18px; font-weight: bold; }");
    btnNextHand->setStyleSheet("QPushButton { background: #10B981; color: white; font-weight: bold; border-radius: 8px; padding: 10px; font-size: 15px; } QPushButton:hover { background: #34D399; }");
    btnSettings->setStyleSheet("QPushButton { background: rgba(0,0,0,0.5); color: #D1D5DB; border-radius: 6px; font-size: 18px; font-weight: bold; } QPushButton:hover { background: rgba(0,0,0,0.7); color: #FBBF24; }");
    btnRules->setStyleSheet("QPushButton { background: rgba(0,0,0,0.5); color: #D1D5DB; border-radius: 6px; font-size: 16px; font-weight: bold; } QPushButton:hover { background: rgba(0,0,0,0.7); color: #FBBF24; }");

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
    btnBackMenu->setGeometry(15, 15, 100, 35);
    btnSettings->setGeometry(width() - 55, 15, 40, 35); // В правом верхнем углу
    btnRules->setGeometry(width() - 100, 15, 40, 35);
    lblStatus->setGeometry(130, 15, width() - 150, 40);
    btnNextHand->setGeometry(width() / 2 - 125, height() / 2 + 70, 250, 55);
}

void BaseTableWidget::drawTableFelt(QPainter& p) {
    p.fillRect(rect(), QColor(20, 20, 20));
    QRect feltRect = rect().adjusted(25, 25, -25, -25);
    QPainterPath tablePath;
    tablePath.addRoundedRect(feltRect, 200, 200);

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

    p.setPen(QPen(QColor(0, 0, 0, 120), 6));
    p.drawPath(tablePath);
}

void BaseTableWidget::drawGameOverBanner(QPainter& p, const QString& message) {
    p.fillRect(rect(), QColor(0, 0, 0, 110));
    QRect bannerRect(width() / 2 - 220, height() / 2 - 125, 440, 60);
    p.setBrush(QColor(15, 23, 42, 240));
    p.setPen(QPen(QColor(251, 191, 36, 220), 2));
    p.drawRoundedRect(bannerRect, 10, 10);

    p.setPen(QColor(252, 211, 77));
    p.setFont(QFont("Segoe UI", 16, QFont::Bold));
    p.drawText(bannerRect, Qt::AlignCenter | Qt::TextWordWrap, message);
}

QVector<QPoint> BaseTableWidget::getSeatPositions(int numPlayers, int width, int height, int bottomYOffset, int topYOffset) {
    if (numPlayers == 2) {
        return {
            QPoint(width / 2, height - bottomYOffset),
            QPoint(width / 2, topYOffset)
        };
    }
    if (numPlayers == 3) {
        return {
            QPoint(width / 2, height - bottomYOffset),
            QPoint(150, height / 2 - 100),
            QPoint(width - 150, height / 2 - 100)
        };
    }
    return {
        QPoint(width / 2, height - bottomYOffset),
        QPoint(150, height / 2 - 100),
        QPoint(width / 2, topYOffset),
        QPoint(width - 150, height / 2 - 100)
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

    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(rect.adjusted(5, 3, -3, -3), Qt::AlignTop | Qt::AlignLeft, rankTxt + "\n" + suitTxt);

    p.save();
    p.translate(rect.center());
    p.rotate(180);
    p.setFont(QFont("Arial", 12, QFont::Bold));
    QRectF localRect(-rect.width() / 2.0, -rect.height() / 2.0, rect.width(), rect.height());
    p.drawText(localRect.adjusted(5, 3, -3, -3), Qt::AlignTop | Qt::AlignLeft, rankTxt + "\n" + suitTxt);
    p.restore();

    p.setFont(QFont("Arial", 32));
    p.drawText(rect, Qt::AlignCenter, suitTxt);

    p.restore();
}

// ============================================================================
// MAIN MENU WIDGET
// ============================================================================

MainMenuWidget::MainMenuWidget(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    auto* lblTitle = new QLabel(getLocalizedText("ROYAL CARD CLUB", "ROYAL CARD CLUB"), this);
    lblTitle->setStyleSheet("QLabel { color: #FBBF24; font-size: 52px; font-weight: 900; letter-spacing: 4px; }");
    lblTitle->setAlignment(Qt::AlignCenter);

    auto* lblSub = new QLabel(getLocalizedText("♠   ♥   ПОКЕР • ДУРАК • КОЗЁЛ • УНО   ♣   ♦", "♠   ♥   POKER • DURAK • KOZEL • UNO   ♣   ♦"), this);
    lblSub->setStyleSheet("QLabel { color: #D97706; font-size: 13px; font-weight: 800; letter-spacing: 4px; margin-bottom: 15px; }");
    lblSub->setAlignment(Qt::AlignCenter);

    QString frameStyle = "QFrame#panel { background: rgba(20, 27, 44, 0.88); border-radius: 16px; border: 1px solid rgba(251, 191, 36, 0.25); }";
    QString comboStyle = "QComboBox { padding: 8px 14px; border-radius: 8px; background: #182234; color: #F9FAFB; border: 1px solid #374151; font-size: 14px; } "
    "QComboBox:hover { border: 1px solid #F59E0B; } "
    "QComboBox::drop-down { border: none; width: 30px; } "
    "QComboBox QAbstractItemView { background: #182234; color: #F9FAFB; selection-background-color: #2563EB; border: 1px solid #374151; padding: 5px; }";
    QString inputStyle = "QLineEdit { padding: 10px 14px; border-radius: 8px; background: #182234; color: #F9FAFB; border: 1px solid #374151; font-size: 14px; } "
    "QLineEdit:focus { border: 1px solid #F59E0B; }";

    auto* selectFrame = new QFrame(this);
    selectFrame->setObjectName("panel");
    selectFrame->setStyleSheet(frameStyle);
    selectFrame->setMaximumWidth(580);
    selectFrame->setMinimumWidth(460);

    auto* selectLayout = new QVBoxLayout(selectFrame);
    selectLayout->setContentsMargins(24, 15, 24, 15);

    auto* lblSelect = new QLabel(getLocalizedText("Выберите игру:", "Select game:"), selectFrame);
    lblSelect->setStyleSheet("color: #F3F4F6; font-size: 15px; font-weight: bold; margin-bottom: 2px;");

    comboGameType = new QComboBox(selectFrame);
    comboGameType->addItem(getLocalizedText("Покер (Texas Hold'em)", "Poker (Texas Hold'em)"), 0);
    comboGameType->addItem(getLocalizedText("Дурак (Подкидной)", "Durak (Podkidnoy)"), 1);
    comboGameType->addItem(getLocalizedText("Козёл", "Kozel"), 2);
    comboGameType->addItem(getLocalizedText("Уно", "UNO"), 3);
    comboGameType->setStyleSheet(comboStyle);
    comboGameType->setCursor(Qt::PointingHandCursor);
    comboGameType->setFixedHeight(42);

    selectLayout->addWidget(lblSelect);
    selectLayout->addWidget(comboGameType);

    auto* botFrame = new QFrame(this);
    botFrame->setObjectName("panel");
    botFrame->setStyleSheet(frameStyle);
    botFrame->setMaximumWidth(580);
    botFrame->setMinimumWidth(460);

    auto* botLayout = new QVBoxLayout(botFrame);
    botLayout->setContentsMargins(24, 15, 24, 18);
    botLayout->setSpacing(10);

    auto* comboLayout = new QHBoxLayout();
    auto* lblBots = new QLabel(getLocalizedText("Соперники:", "Opponents:"), botFrame);
    lblBots->setStyleSheet("color: #9CA3AF; font-size: 14px; font-weight: bold;");

    comboBots = new QComboBox(botFrame);
    comboBots->addItem(getLocalizedText("1 Бот (Голова к голове)", "1 Bot (Heads Up)"), 1);
    comboBots->addItem(getLocalizedText("2 Бота (3 Макс.)", "2 Bots (3 Max)"), 2);
    comboBots->addItem(getLocalizedText("3 Бота (4 Макс.)", "3 Bots (4 Max)"), 3);
    comboBots->setStyleSheet(comboStyle);
    comboBots->setCursor(Qt::PointingHandCursor);
    comboBots->setFixedHeight(42);

    comboLayout->addWidget(lblBots);
    comboLayout->addWidget(comboBots);

    btnStartBotGame = new QPushButton(getLocalizedText("Начать игру", "Start Game"), botFrame);
    btnStartBotGame->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #059669, stop:1 #10B981); color: white; font-weight: bold; font-size: 15px; padding: 12px; border-radius: 8px; border: none; } "
    "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #10B981, stop:1 #34D399); }");
    btnStartBotGame->setCursor(Qt::PointingHandCursor);

    botLayout->addWidget(new QLabel(getLocalizedText("Одиночная игра (с Ботами):", "Singleplayer (vs Bots):"), botFrame));
    botLayout->addLayout(comboLayout);
    botLayout->addWidget(btnStartBotGame);

    auto* netFrame = new QFrame(this);
    netFrame->setObjectName("panel");
    netFrame->setStyleSheet(frameStyle);
    netFrame->setMaximumWidth(580);
    netFrame->setMinimumWidth(460);

    auto* netLayout = new QVBoxLayout(netFrame);
    netLayout->setContentsMargins(24, 15, 24, 18);
    netLayout->setSpacing(10);

    btnHostServer = new QPushButton(getLocalizedText("Создать сервер", "Create Server"), netFrame);
    btnHostServer->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1D4ED8, stop:1 #3B82F6); color: white; font-weight: bold; font-size: 15px; padding: 12px; border-radius: 8px; border: none; } "
    "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2563EB, stop:1 #60A5FA); }");
    btnHostServer->setCursor(Qt::PointingHandCursor);

    auto* connectLayout = new QHBoxLayout();
    ipInput = new QLineEdit(netFrame);
    ipInput->setPlaceholderText("127.0.0.1");
    ipInput->setText("127.0.0.1");
    ipInput->setStyleSheet(inputStyle);
    ipInput->setFixedHeight(44);

    btnConnectIP = new QPushButton(getLocalizedText("Подключиться", "Connect"), netFrame);
    btnConnectIP->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #B45309, stop:1 #F59E0B); color: white; font-weight: bold; font-size: 15px; padding: 12px; border-radius: 8px; border: none; } "
    "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #D97706, stop:1 #FBBF24); }");
    btnConnectIP->setCursor(Qt::PointingHandCursor);
    btnConnectIP->setFixedHeight(44);

    connectLayout->addWidget(ipInput, 1);
    connectLayout->addWidget(btnConnectIP, 1);

    netLayout->addWidget(new QLabel(getLocalizedText("Сетевая игра (LAN / IP):", "Multiplayer (LAN / IP):"), netFrame));
    netLayout->addWidget(btnHostServer);
    netLayout->addLayout(connectLayout);

    btnSettings = new QPushButton(getLocalizedText("Настройки", "Settings"), this);
    btnSettings->setMaximumWidth(580);
    btnSettings->setMinimumWidth(460);
    btnSettings->setStyleSheet(
        "QPushButton { background: rgba(20, 27, 44, 0.88); color: #F3F4F6; font-weight: bold; font-size: 14px; padding: 12px; border-radius: 12px; border: 1px solid rgba(251, 191, 36, 0.3); } "
        "QPushButton:hover { background: rgba(30, 41, 65, 0.95); border: 1px solid #F59E0B; }"
    );
    btnSettings->setCursor(Qt::PointingHandCursor);

    btnRules = new QPushButton(getLocalizedText("Правила игры", "Game Rules"), this);
    btnRules->setMaximumWidth(580);
    btnRules->setMinimumWidth(460);
    btnRules->setStyleSheet(
        "QPushButton { background: rgba(20, 27, 44, 0.88); color: #F3F4F6; font-weight: bold; font-size: 14px; padding: 12px; border-radius: 12px; border: 1px solid rgba(251, 191, 36, 0.3); } "
        "QPushButton:hover { background: rgba(30, 41, 65, 0.95); border: 1px solid #F59E0B; }"
    );
    btnRules->setCursor(Qt::PointingHandCursor);

    connect(btnSettings, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playSound(SoundEffect::ButtonClick);
        SettingsDialog dialog(this);
        dialog.exec();
    });

    mainLayout->addWidget(lblTitle);
    mainLayout->addWidget(lblSub);
    mainLayout->addWidget(selectFrame, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(botFrame, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(netFrame, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(btnSettings, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(btnRules, 0, Qt::AlignHCenter);
    mainLayout->addStretch();
}

void MainMenuWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRadialGradient bg(width() / 2, height() / 2, qMax(width(), height()) * 0.65);
    bg.setColorAt(0.0, QColor(28, 42, 68));
    bg.setColorAt(0.5, QColor(14, 21, 35));
    bg.setColorAt(1.0, QColor(6, 9, 16));
    p.fillRect(rect(), bg);

    int suitSize = qBound(140, int(height() * 0.22), 240);
    int panelW = qMin(580, int(width() * 0.8));
    double leftX = (width() - panelW) / 2.0 - suitSize * 0.5;
    double rightX = (width() + panelW) / 2.0 + suitSize * 0.5;

    auto drawWatermark = [&](double x, double y, const QString& suit, double angle) {
        p.save();
        p.translate(x, y);
        p.rotate(angle);
        p.setFont(QFont("Arial", suitSize, QFont::Bold));
        p.setPen(QColor(251, 191, 36, 25));
        p.drawText(-suitSize / 2, suitSize / 2, suit);
        p.restore();
    };

    drawWatermark(leftX, height() * 0.30, "♠", -14);
    drawWatermark(rightX, height() * 0.26, "♥", 12);
    drawWatermark(leftX - 20, height() * 0.76, "♣", 16);
    drawWatermark(rightX + 20, height() * 0.72, "♦", -10);
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

    QString btnStyle = "QPushButton { background: %1; color: white; font-weight: bold; font-size: 15px; border-radius: 8px; border: none; } QPushButton:hover { background: %2; }";
    btnFold->setStyleSheet(btnStyle.arg("#EF4444", "#F87171"));
    btnCall->setStyleSheet(btnStyle.arg("#3B82F6", "#60A5FA"));
    btnRaise->setStyleSheet(btnStyle.arg("#F59E0B", "#FBBF24"));
    btnStartNetGame->setStyleSheet(btnStyle.arg("#8B5CF6", "#A78BFA"));

    lblRaiseAmount->setStyleSheet("QLabel { color: #FCD34D; font-size: 16px; font-weight: bold; background: rgba(0,0,0,0.5); border-radius: 4px; padding: 4px; }");
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
        if (netManager && netManager->isHost) {
            int activeClients = netManager->getActiveClientCount();
            if (netManager->isNetworkGame && activeClients == 0) {
                netManager->isLobby = true;
                engine.gameOver = false;
                engine.players.resize(1);
                engine.players[0].id = 0;
                engine.players[0].name = AppSettings::instance().nickname;
                engine.players[0].avatar = static_cast<int>(AppSettings::instance().avatar);
                engine.communityCards.clear();
                engine.pot = 0;

                engine.statusMessage = QString(getLocalizedText("ЛОББИ: 1/%1 игроков. Ожидание...", "LOBBY: 1/%1 players. Waiting...")).arg(PokerConfig::MAX_PLAYERS);
                lblStatus->setText(engine.statusMessage);
                broadcastNetState();
                updateUI();
                return;
            }

            if (engine.countSolventPlayers() < 2) engine.resetGame();
            else engine.startNewHand();

            broadcastNetState();
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
        if (netManager && netManager->isHost && engine.gameOver && !netManager->isLobby && engine.countSolventPlayers() >= 2) {
            handleNextHandLambda();
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
            if (engine.countSolventPlayers() < 2) {
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
        if (!engine.statusMessage.isEmpty()) {
            lblStatus->setText(engine.statusMessage);
        }
    } else if (netManager && netManager->isHost) {
        lblStatus->setText(QString(getLocalizedText("ЛОББИ: %1/%2 игроков. Ожидание...", "LOBBY: %1/%2 players. Waiting...")).arg(activeClients + 1).arg(PokerConfig::MAX_PLAYERS));
    }

    update();
}

void PokerWidget::resizeEvent(QResizeEvent* ev) {
    BaseTableWidget::resizeEvent(ev);
    int btnY = height() - 75;
    btnFold->setGeometry(width() / 2 - 250, btnY, 130, 50);
    btnCall->setGeometry(width() / 2 - 100, btnY, 140, 50);

    raiseSlider->setGeometry(width() / 2 + 60, btnY, 140, 20);
    lblRaiseAmount->setGeometry(width() / 2 + 60, btnY + 25, 140, 25);
    btnRaise->setGeometry(width() / 2 + 220, btnY, 130, 50);

    btnStartNetGame->setGeometry(width() / 2 - 150, height() / 2 + 50, 300, 60);
}

void PokerWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    drawTableFelt(p);

    if (netManager && netManager->isNetworkGame && netManager->isLobby) {
        p.setPen(QColor(255, 215, 0));
        p.setFont(QFont("Segoe UI", 24, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, lblStatus->text());
        return;
    }

    if (!engine.players.isEmpty()) {
        p.setPen(QColor(252, 211, 77));
        p.setFont(QFont("Segoe UI", 18, QFont::Bold));
        p.drawText(QRect(0, height() / 2 - 120, width(), 30), Qt::AlignCenter, QString("POT: $%1").arg(engine.pot));

        int cardW = 80, cardH = 115;
        int commStartX = width() / 2 - (5 * 90) / 2;

        for (int i = 0; i < 5; ++i) {
            QRect cRect(commStartX + i * 90, height() / 2 - 55, cardW, cardH);
            if (i < engine.communityCards.size()) drawCard(p, cRect, &engine.communityCards[i], true);
            else {
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
    QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), 130, 100);

    for (int i = 0; i < numPlayers; ++i) {
        int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        QPoint pos = seatPos[displayIdx];
        auto& plr = engine.players[i];

        if (engine.currentTurnIdx == i && !engine.gameOver && !plr.hasFolded && !plr.isBankrupt && !plr.isDisconnected) {
            p.setPen(QPen(QColor(59, 130, 246, 220), 4));
            p.drawRoundedRect(pos.x() - 105, pos.y() - 55, 210, 70, 10, 10);
        }

        p.setBrush(QColor(15, 25, 35, 230));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.drawRoundedRect(pos.x() - 100, pos.y() - 50, 200, 60, 8, 8);

        if (engine.dealerIdx == i) {
            p.setBrush(Qt::white);
            p.setPen(QPen(QColor(0, 0, 0), 1));
            p.drawEllipse(pos.x() - 115, pos.y() - 25, 22, 22);
            p.setPen(Qt::black);
            p.setFont(QFont("Segoe UI", 12, QFont::Bold));
            p.drawText(QRect(pos.x() - 115, pos.y() - 25, 22, 22), Qt::AlignCenter, "D");
        }

        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 12, QFont::Bold));

        QString nameWithAvatar = getAvatarEmojiById(plr.avatar) + " " + plr.name;
        p.drawText(QRect(pos.x() - 90, pos.y() - 45, 180, 25), Qt::AlignLeft | Qt::AlignVCenter, nameWithAvatar);

        if (displayIdx == 0 && AppSettings::instance().showPokerHandHint && !plr.hasFolded && !plr.holeCards.isEmpty() && engine.phase != PREFLOP) {
            QVector<Card> allCards = plr.holeCards;
            allCards.append(engine.communityCards);
            HandValue hv = evaluate7Cards(allCards);

            p.setPen(QColor(251, 191, 36));
            p.setFont(QFont("Segoe UI", 11, QFont::Bold));
            p.drawText(QRect(pos.x() - 120, pos.y() + 15, 240, 25), Qt::AlignCenter, QString("[%1]").arg(hv.name));
        }

        p.setPen(QColor(167, 243, 208));
        p.drawText(QRect(pos.x() - 90, pos.y() - 20, 180, 25), Qt::AlignLeft | Qt::AlignVCenter, QString("$%1").arg(plr.balance));

        if (plr.currentBet > 0) {
            p.setPen(QColor(253, 230, 138));
            p.drawText(QRect(pos.x() - 90, pos.y() - 20, 180, 25), Qt::AlignRight | Qt::AlignVCenter, QString("Bet: %1").arg(plr.currentBet));
        }

        if (plr.isDisconnected) {
            p.setPen(QColor(107, 114, 128));
            p.drawText(QRect(pos.x() - 90, pos.y() - 45, 180, 25), Qt::AlignRight | Qt::AlignVCenter, getLocalizedText("ОТКЛЮЧЕН", "OFFLINE"));
        } else if (plr.isBankrupt) {
            p.setPen(QColor(156, 163, 175));
            p.drawText(QRect(pos.x() - 90, pos.y() - 45, 180, 25), Qt::AlignRight | Qt::AlignVCenter, getLocalizedText("РАЗОРЁН", "BANKRUPT"));
        } else if (plr.isAllIn) {
            p.setPen(QColor(248, 113, 113));
            p.drawText(QRect(pos.x() - 90, pos.y() - 45, 180, 25), Qt::AlignRight | Qt::AlignVCenter, getLocalizedText("ALL-IN", "ALL-IN"));
        } else if (plr.hasFolded) {
            p.setPen(QColor(156, 163, 175));
            p.drawText(QRect(pos.x() - 90, pos.y() - 45, 180, 25), Qt::AlignRight | Qt::AlignVCenter, getLocalizedText("FOLD", "FOLD"));
        }

        if (!plr.hasFolded && !plr.isBankrupt && !plr.isDisconnected) {
            int cardsStartX = pos.x() - cardW + 10;
            int cardsY = (displayIdx == 0) ? pos.y() - 170 : pos.y() + 25;

            for (int c = 0; c < plr.holeCards.size(); ++c) {
                bool faceUp = (i == engine.myIdx || engine.phase == SHOWDOWN || engine.gameOver);
                drawCard(p, QRect(cardsStartX + c * (cardW + 10), cardsY, cardW, cardH), &plr.holeCards[c], faceUp);
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
    btnPass->setGeometry(width() - 270, height() - 65, 120, 45);
    btnTake->setGeometry(width() - 140, height() - 65, 120, 45);
}

void DurakWidget::mouseMoveEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty() || engine.myIdx >= engine.players.size()) return;
    auto& myHand = engine.players[engine.myIdx].hand;
    int cardW = 80, cardH = 115;
    int handY = height() - 235;
    int stepX = qMin(50, (width() - 300) / qMax(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    int newHovered = -1;
    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = (i == selectedHandCardIdx) ? -25 : ((i == hoveredHandCardIdx) ? -12 : 0);
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
    int cardW = 80, cardH = 115;
    int handY = height() - 235;
    int stepX = qMin(50, (width() - 300) / qMax(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = (i == selectedHandCardIdx) ? -25 : ((i == hoveredHandCardIdx) ? -12 : 0);
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
        int tableY = height() / 2 - 55;
        int totalTableW = engine.table.size() * 110;
        int tableStartX = (width() - totalTableW) / 2;
        for (int t = 0; t < engine.table.size(); ++t) {
            if (QRect(tableStartX + t * 110, tableY, cardW, cardH).contains(ev->pos()) && !engine.table[t].isDefended) {
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

    if (!engine.players.isEmpty()) {
        int cardW = 80, cardH = 115;

        if (!engine.deck.isEmpty()) {
            drawCard(p, QRect(15, 125, cardH, cardW), &engine.trumpCard, true);
            drawCard(p, QRect(55, 95, cardW, cardH), nullptr, false);

            p.setPen(Qt::white);
            p.setFont(QFont("Segoe UI", 11, QFont::Bold));
            p.drawText(35, 225, QString(getLocalizedText("Карт: %1", "Cards: %1")).arg(engine.deck.size()));
        } else {
            p.setPen(QColor(255, 235, 59));
            p.setFont(QFont("Segoe UI", 13, QFont::Bold));
            static const QString suitsStr[] = { "♥", "♦", "♣", "♠" };
            p.drawText(35, 120, QString(getLocalizedText("Козырь: %1", "Trump: %1")).arg(suitsStr[engine.trumpCard.suit]));
        }

        if (engine.bitoCount > 0) {
            p.save();
            p.translate(width() - 80, 150);
            p.rotate(15);
            drawCard(p, QRect(-cardW / 2, -cardH / 2, cardW, cardH), nullptr, false);
            p.restore();

            p.setPen(Qt::white);
            p.setFont(QFont("Segoe UI", 11, QFont::Bold));
            p.drawText(width() - 110, 225, QString(getLocalizedText("Бито: %1", "Discards: %1")).arg(engine.bitoCount));
        }

        drawPlayers(p, cardW, cardH);

        int tableY = height() / 2 - 55;
        int totalTableW = engine.table.size() * 110;
        int tableStartX = (width() - totalTableW) / 2;
        for (int t = 0; t < engine.table.size(); ++t) {
            int cardX = tableStartX + t * 110;
            drawCard(p, QRect(cardX, tableY, cardW, cardH), &engine.table[t].attack, true);
            if (engine.table[t].isDefended) {
                drawCard(p, QRect(cardX + 22, tableY + 22, cardW, cardH), &engine.table[t].defend, true);
            }
        }

        if (engine.myIdx < engine.players.size()) {
            auto& myHand = engine.players[engine.myIdx].hand;
            int handY = height() - 235;
            int stepX = qMin(50, (width() - 300) / qMax(1, myHand.size()));
            int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;
            for (int i = 0; i < myHand.size(); ++i) {
                bool isSelected = (i == selectedHandCardIdx);
                bool isHovered = (i == hoveredHandCardIdx);
                int offsetY = isSelected ? -25 : (isHovered ? -12 : 0);
                drawCard(p, QRect(startX + i * stepX, handY + offsetY, cardW, cardH), &myHand[i], true, isSelected);
            }
        }
    }

    if (engine.gameOver) {
        drawGameOverBanner(p, engine.statusMessage);
    }
}

void DurakWidget::drawPlayers(QPainter& p, int cardW, int cardH) {
    int numPlayers = engine.players.size();
    QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), 75, 80);

    for (int i = 0; i < numPlayers; ++i) {
        int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        QPoint pos = seatPos[displayIdx];
        auto& opp = engine.players[i];

        if ((engine.attackerIdx == i || engine.defenderIdx == i) && !engine.gameOver) {
            p.setPen(QPen(QColor(59, 130, 246, 220), 3));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(pos.x() - 95, pos.y() - 30, 190, 50, 10, 10);
        }

        p.setBrush(QColor(15, 25, 35, 230));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.drawRoundedRect(pos.x() - 90, pos.y() - 25, 180, 45, 8, 8);

        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 11, QFont::Bold));
        QString nameWithAvatar = getAvatarEmojiById(opp.avatar) + " " + opp.name;
        p.drawText(QRect(pos.x() - 80, pos.y() - 20, 160, 35), Qt::AlignCenter, nameWithAvatar);

        if (displayIdx != 0) {
            int handSize = opp.hand.size();
            int startX = pos.x() - (handSize * 15 + (cardW - 15)) / 2;
            for (int c = 0; c < handSize; ++c) {
                drawCard(p, QRect(startX + c * 15, pos.y() + 25, cardW - 25, cardH - 35), nullptr, false);
            }
        }
    }
}

// ============================================================================
// KOZEL WIDGET
// ============================================================================

KozelWidget::KozelWidget(NetworkManager* netMgr, QWidget* parent) : BaseTableWidget(parent), netManager(netMgr) {
    btnPlayCards = new QPushButton(getLocalizedText("СДЕЛАТЬ ХОД", "PLAY CARDS"), this);
    btnPlayCards->setStyleSheet("QPushButton { background: #10B981; color: white; font-weight: bold; border-radius: 6px; padding: 10px; font-size: 14px; }");
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
    btnPlayCards->setGeometry(width() - 180, height() - 65, 150, 45);
}

void KozelWidget::mouseMoveEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty() || engine.myIdx >= engine.players.size()) return;
    auto& myHand = engine.players[engine.myIdx].hand;
    int cardW = 80, cardH = 115;
    int handY = height() - 235;
    int stepX = qMin(60, (width() - 300) / qMax(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    int newHovered = -1;
    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = selectedHandCardIndices.contains(i) ? -25 : ((i == hoveredHandCardIdx) ? -12 : 0);
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
    int cardW = 80, cardH = 115;
    int handY = height() - 235;
    int stepX = qMin(60, (width() - 300) / qMax(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = selectedHandCardIndices.contains(i) ? -25 : ((i == hoveredHandCardIdx) ? -12 : 0);
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

    if (!engine.players.isEmpty()) {
        int cardW = 80, cardH = 115;
        p.setPen(QColor(255, 215, 0));
        p.setFont(QFont("Segoe UI", 14, QFont::Bold));
        static const QString suitsStr[] = { "♥", "♦", "♣", "♠" };
        p.drawText(35, 80, QString(getLocalizedText("Козырь: %1", "Trump: %1")).arg(suitsStr[engine.trumpSuit]));

        drawPlayers(p, cardW, cardH);

        int trickStartX = width() / 2 - (engine.currentTrick.size() * 45) / 2;
        for (int t = 0; t < engine.currentTrick.size(); ++t) {
            drawCard(p, QRect(trickStartX + t * 45, height() / 2 - 55, cardW, cardH), &engine.currentTrick[t].second, true);
        }

        if (engine.myIdx < engine.players.size()) {
            auto& myHand = engine.players[engine.myIdx].hand;
            int handY = height() - 235;
            int stepX = qMin(60, (width() - 300) / qMax(1, myHand.size()));
            int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;
            for (int i = 0; i < myHand.size(); ++i) {
                bool isSelected = selectedHandCardIndices.contains(i);
                bool isHovered  = (i == hoveredHandCardIdx);
                int offsetY     = isSelected ? -25 : (isHovered ? -12 : 0);
                drawCard(p, QRect(startX + i * stepX, handY + offsetY, cardW, cardH), &myHand[i], true, isSelected);
            }
        }
    }

    if (engine.gameOver) {
        drawGameOverBanner(p, engine.statusMessage);
    }
}

void KozelWidget::drawPlayers(QPainter& p, int cardW, int cardH) {
    int numPlayers = engine.players.size();
    QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), 75, 80);

    for (int i = 0; i < numPlayers; ++i) {
        int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        QPoint pos = seatPos[displayIdx];
        auto& plr = engine.players[i];

        if (engine.currentTurnIdx == i && !engine.gameOver) {
            p.setPen(QPen(QColor(59, 130, 246, 220), 3));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(pos.x() - 80, pos.y() - 30, 160, 50, 10, 10);
        }

        p.setBrush(QColor(15, 25, 35, 230));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.drawRoundedRect(pos.x() - 75, pos.y() - 25, 150, 45, 8, 8);

        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 11, QFont::Bold));
        QString nameWithAvatar = getAvatarEmojiById(plr.avatar) + " " + plr.name;
        p.drawText(QRect(pos.x() - 70, pos.y() - 20, 140, 20), Qt::AlignLeft, nameWithAvatar);

        p.setPen(QColor(167, 243, 208));
        p.drawText(QRect(pos.x() - 70, pos.y() - 0, 140, 20), Qt::AlignLeft, QString(getLocalizedText("Очки: %1", "Points: %1")).arg(plr.pointsCollected));

        if (plr.pointsCollected > 0) {
            p.save();
            p.translate(pos.x() + 65, pos.y() - 10);
            p.rotate(10);
            drawCard(p, QRect(-20, -25, 40, 55), nullptr, false);
            p.restore();
        }

        if (displayIdx != 0) {
            int handSize = plr.hand.size();
            int startX = pos.x() - (handSize * 15 + (cardW - 15)) / 2;
            for (int c = 0; c < handSize; ++c) {
                drawCard(p, QRect(startX + c * 15, pos.y() + 25, cardW - 25, cardH - 35), nullptr, false);
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

    btnDrawCard->setStyleSheet("QPushButton { background: #2563EB; color: white; font-weight: bold; border-radius: 8px; padding: 10px; font-size: 14px; } QPushButton:hover { background: #3B82F6; }");
    btnPass->setStyleSheet("QPushButton { background: #64748B; color: white; font-weight: bold; border-radius: 8px; padding: 10px; font-size: 14px; } QPushButton:hover { background: #94A3B8; }");
    btnUno->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #DC2626, stop:1 #F59E0B); color: white; font-weight: 900; font-size: 15px; border-radius: 8px; border: 2px solid #FDE047; padding: 8px; } QPushButton:hover { background: #EF4444; }");
    btnCatchUno->setStyleSheet("QPushButton { background: #D97706; color: white; font-weight: bold; font-size: 13px; border-radius: 8px; border: 1px solid #FCD34D; padding: 8px; } QPushButton:hover { background: #F59E0B; }");

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
    connect(arrowAnimTimer, &QTimer::timeout, this, [this]() {
        if (!engine.gameOver && isVisible()) {
            arrowAnimAngle += engine.direction * 1.0; // Плавный шаг 1 градус за кадр
            if (arrowAnimAngle >= 360.0) arrowAnimAngle -= 360.0;
            if (arrowAnimAngle < 0.0)    arrowAnimAngle += 360.0;
            update();
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
        // Кнопка штрафа видна только если нечем ответить (+2 / +4)
        btnDrawCard->setVisible(isMyTurn && !hasMove);
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
        btnUno->setVisible((isMyTurn && handCount <= 2 && !engine.players[engine.myIdx].saidUno) || isVulnerable);
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
        lblStatus->setText(engine.statusMessage);
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
    btnDrawCard->setGeometry(width() - 180, height() - 65, 150, 45);
    btnPass->setGeometry(width() - 180, height() - 65, 150, 45);
    btnUno->setGeometry(width() - 320, height() - 65, 130, 45);
    btnCatchUno->setGeometry(width() - 320, height() - 65, 130, 45);
    colorPickerWidget->setGeometry(width() / 2 - 125, height() - 110, 250, 48);

    drawDeckRect = QRect(30, 100, 80, 115);
}

void UnoWidget::drawUnoCard(QPainter& p, const QRect& rect, const UnoCard* card, bool faceUp, bool selected) {
    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    // Тень под картой
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 120));
    p.drawRoundedRect(rect.translated(3, 4), 8, 8);

    QPainterPath path;
    path.addRoundedRect(rect, 8, 8);

    // =========================================================================
    // РУБАШКА КАРТЫ
    // =========================================================================
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

        QFont unoFont("Arial Black", 15, QFont::Black);
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

    // =========================================================================
    // ЛИЦЕВАЯ СТОРОНА КАРТЫ
    // =========================================================================
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

        // Верхняя половина (Красный слева, Синий справа)
        p.save();
        p.setClipRect(QRectF(rect.left(), rect.top(), rect.width(), rect.height() / 2.0));
        p.translate(rect.center());
        p.rotate(34);
        p.setBrush(QColor(220, 38, 38)); p.drawPie(ovalRect, 90 * 16, 180 * 16);  // Лево: Красный
        p.setBrush(QColor(37, 99, 235));  p.drawPie(ovalRect, -90 * 16, 180 * 16); // Право: Синий
        p.restore();

        // Нижняя половина (Жёлтый слева, Зелёный справа)
        p.save();
        p.setClipRect(QRectF(rect.left(), rect.center().y(), rect.width(), rect.height() / 2.0));
        p.translate(rect.center());
        p.rotate(34);
        p.setBrush(QColor(234, 179, 8));  p.drawPie(ovalRect, 90 * 16, 180 * 16);  // Лево: Жёлтый
        p.setBrush(QColor(22, 163, 74));  p.drawPie(ovalRect, -90 * 16, 180 * 16); // Право: Зелёный
        p.restore();

        // Белая окантовка поверх
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

    // 2. Центральный символ (под углом 0°)
    if (card->color == UnoWild) {
        if (card->value == UnoWildDrawFour) {
            QFont centerFont("Arial Black", 26, QFont::Black);
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

        QFont centerFont("Arial Black", centerTxt.length() > 1 ? 22 : 30, QFont::Black);
        centerFont.setItalic(true);
        p.setFont(centerFont);

        p.setPen(QColor(0, 0, 0, 80));
        p.drawText(rect.translated(2, 2), Qt::AlignCenter, centerTxt);

        p.setPen(Qt::white);
        p.drawText(rect, Qt::AlignCenter, centerTxt);
    }

    p.restore(); // Сброс clipPath

    // 3. Угловые индексы (для Wild — мини 4-цветные овалы с горизонтальным разделением)
    auto drawMiniWildOval = [&](const QPointF& pt) {
        QRectF miniRect(-4.5, -7.5, 9, 15);
        p.setPen(Qt::NoPen);

        // Верхняя половина
        p.save();
        p.setClipRect(QRectF(pt.x() - 10, pt.y() - 10, 20, 10));
        p.translate(pt);
        p.rotate(34);
        p.setBrush(QColor(220, 38, 38)); p.drawPie(miniRect, 90 * 16, 180 * 16);
        p.setBrush(QColor(37, 99, 235));  p.drawPie(miniRect, -90 * 16, 180 * 16);
        p.restore();

        // Нижняя половина
        p.save();
        p.setClipRect(QRectF(pt.x() - 10, pt.y(), 20, 10));
        p.translate(pt);
        p.rotate(34);
        p.setBrush(QColor(234, 179, 8));  p.drawPie(miniRect, 90 * 16, 180 * 16);
        p.setBrush(QColor(22, 163, 74));  p.drawPie(miniRect, -90 * 16, 180 * 16);
        p.restore();

        // Окантовка
        p.save();
        p.translate(pt);
        p.rotate(34);
        p.setPen(QPen(Qt::white, 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(miniRect);
        p.restore();
    };

    if (card->color == UnoWild && card->value == UnoWildCard) {
        drawMiniWildOval(QPointF(rect.left() + 10, rect.top() + 13));
        drawMiniWildOval(QPointF(rect.right() - 10, rect.bottom() - 13));
    } else {
        QString cornerTxt;
        if (card->value <= UnoNine) cornerTxt = QString::number(card->value);
        else if (card->value == UnoSkip) cornerTxt = "⊘";
        else if (card->value == UnoReverse) cornerTxt = "⇄";
        else if (card->value == UnoDrawTwo) cornerTxt = "+2";
        else if (card->value == UnoWildDrawFour) cornerTxt = "+4";

        QFont cornerFont("Arial Black", 10, QFont::Bold);
        cornerFont.setItalic(true);
        p.setFont(cornerFont);

        // Верхний левый
        p.setPen(QColor(0, 0, 0, 100));
        p.drawText(rect.adjusted(6, 4, -4, -4), Qt::AlignTop | Qt::AlignLeft, cornerTxt);
        p.setPen(Qt::white);
        p.drawText(rect.adjusted(5, 3, -4, -4), Qt::AlignTop | Qt::AlignLeft, cornerTxt);

        // Нижний правый
        p.save();
        p.translate(rect.center());
        p.rotate(180);
        QRectF localRect(-rect.width() / 2.0, -rect.height() / 2.0, rect.width(), rect.height());
        p.setPen(QColor(0, 0, 0, 100));
        p.drawText(localRect.adjusted(6, 4, -4, -4), Qt::AlignTop | Qt::AlignLeft, cornerTxt);
        p.setPen(Qt::white);
        p.drawText(localRect.adjusted(5, 3, -4, -4), Qt::AlignTop | Qt::AlignLeft, cornerTxt);
        p.restore();
    }

    p.restore();
}

void UnoWidget::mouseMoveEvent(QMouseEvent* ev) {
    if (engine.gameOver || engine.players.isEmpty() || engine.myIdx >= engine.players.size()) return;
    auto& myHand = engine.players[engine.myIdx].hand;
    int cardW = 80, cardH = 115;
    int handY = height() - 235;
    int stepX = qMin(50, (width() - 300) / qMax(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    int newHovered = -1;
    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = (i == selectedHandCardIdx) ? -25 : ((i == hoveredHandCardIdx) ? -12 : 0);
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
    int cardW = 80, cardH = 115;
    int handY = height() - 235;
    int stepX = qMin(50, (width() - 300) / qMax(1, myHand.size()));
    int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;

    bool clickedOnCard = false;

    for (int i = myHand.size() - 1; i >= 0; --i) {
        int offsetY = (i == selectedHandCardIdx) ? -25 : ((i == hoveredHandCardIdx) ? -12 : 0);
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

    // Клик мимо карт сбрасывает меню выбора цвета
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

    if (!engine.players.isEmpty()) {
        int cardW = 80, cardH = 115;

        // --- 1. КОЛОДА ВВЕРХУ СЛЕВА (КАК В ДУРАКЕ) ---
        int deckX = 30;
        int deckY = 100;
        drawDeckRect = QRect(deckX, deckY, cardW, cardH);

        if (engine.deck.size() > 1) drawUnoCard(p, QRect(deckX + 4, deckY + 4, cardW, cardH), nullptr, false);
        if (engine.deck.size() > 5) drawUnoCard(p, QRect(deckX + 2, deckY + 2, cardW, cardH), nullptr, false);
        drawUnoCard(p, drawDeckRect, nullptr, false);

        QRect badgeRect(deckX - 5, deckY + cardH + 8, cardW + 10, 22);
        p.setBrush(QColor(15, 23, 42, 220));
        p.setPen(QPen(QColor(251, 191, 36, 180), 1));
        p.drawRoundedRect(badgeRect, 6, 6);
        p.setFont(QFont("Segoe UI", 10, QFont::Bold));
        p.setPen(Qt::white);
        p.drawText(badgeRect, Qt::AlignCenter, QString(getLocalizedText("Карт: %1", "Cards: %1")).arg(engine.deck.size()));

        // --- 2. СБРОС И СТРЕЛКИ НАПРАВЛЕНИЯ В ЦЕНТРЕ ---
        drawCenterDiscard(p, cardW, cardH);

        // --- 3. ИГРОКИ ---
        drawPlayers(p, cardW, cardH);

        // --- 4. РУКА ИГРОКА ---
        if (engine.myIdx < engine.players.size()) {
            auto& myHand = engine.players[engine.myIdx].hand;
            int handY = height() - 235;
            int stepX = qMin(50, (width() - 300) / qMax(1, myHand.size()));
            int startX = (width() - (myHand.size() * stepX + (cardW - stepX))) / 2;
            for (int i = 0; i < myHand.size(); ++i) {
                bool isSelected = (i == selectedHandCardIdx);
                bool isHovered  = (i == hoveredHandCardIdx);
                int offsetY     = isSelected ? -25 : (isHovered ? -12 : 0);
                drawUnoCard(p, QRect(startX + i * stepX, handY + offsetY, cardW, cardH), &myHand[i], true, isSelected);
            }
        }
    }

    if (engine.gameOver) {
        drawGameOverBanner(p, engine.statusMessage);
    }
}

void UnoWidget::drawCenterDiscard(QPainter& p, int cardW, int cardH) {
    QPoint center(width() / 2, height() / 2 - 25);

    // Цвет активной масти для стрелок
    const QColor arrowColors[] = { QColor(220, 38, 38), QColor(234, 179, 8), QColor(22, 163, 74), QColor(37, 99, 235) };
    QColor curCol = arrowColors[engine.currentColor];

    // Круговые стрелки направления вокруг сброса
    int arrowRadius = 110;
    p.setPen(QPen(QColor(curCol.red(), curCol.green(), curCol.blue(), 70), 3, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(center, arrowRadius, arrowRadius);

    p.save();
    p.translate(center);
    p.setPen(QPen(curCol, 2));
    p.setBrush(curCol);

    for (int a = 0; a < 360; a += 180) {
        p.save();
        p.rotate(a + arrowAnimAngle); // Позиция стрелок динамически сдвигается
        p.translate(0, -arrowRadius);
        QPolygonF arrow;
        if (engine.direction == 1) arrow << QPointF(-9, -7) << QPointF(9, 0) << QPointF(-9, 7);
        else                       arrow << QPointF(9, -7) << QPointF(-9, 0) << QPointF(9, 7);
        p.drawPolygon(arrow);
        p.restore();
    }
    p.restore();

    // Сброс в самом центре
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
    QVector<QPoint> seatPos = getSeatPositions(numPlayers, width(), height(), 75, 80);

    for (int i = 0; i < numPlayers; ++i) {
        int displayIdx = (i - engine.myIdx + numPlayers) % numPlayers;
        QPoint pos = seatPos[displayIdx];
        auto& opp = engine.players[i];
        int handSize = opp.hand.size();

        // 1. Подсветка активного хода
        if (engine.currentTurnIdx == i && !engine.gameOver) {
            p.setPen(QPen(QColor(59, 130, 246, 220), 3));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(pos.x() - 85, pos.y() - 28, 170, 48, 10, 10);
        }

        // 2. Плашка с именем и аватаром
        p.setBrush(QColor(15, 25, 35, 230));
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.drawRoundedRect(pos.x() - 80, pos.y() - 24, 160, 42, 8, 8);

        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 11, QFont::Bold));
        QString nameWithAvatar = getAvatarEmojiById(opp.avatar) + " " + opp.name;
        p.drawText(QRect(pos.x() - 75, pos.y() - 24, 150, 42), Qt::AlignVCenter | Qt::AlignLeft, nameWithAvatar);

        // 3. Бейдж количества карт / Индикатор UNO! (рисуется ДЛЯ ВСЕХ игроков)
        QRect badgeRect(pos.x() + 88, pos.y() - 24, 42, 42);
        bool isUno = (handSize == 1);
        p.setBrush(isUno ? QColor(220, 38, 38) : QColor(30, 41, 59, 240));
        p.setPen(QPen(isUno ? QColor(254, 240, 138) : QColor(255, 255, 255, 60), isUno ? 2 : 1));
        p.drawRoundedRect(badgeRect, 6, 6);

        p.setFont(QFont("Segoe UI", isUno ? 9 : 11, QFont::Bold));
        p.setPen(Qt::white);
        p.drawText(badgeRect, Qt::AlignCenter, isUno ? "UNO!" : QString("x%1").arg(handSize));

        // 4. Веер закрытых карт (только для соперников)
        if (displayIdx != 0) {
            int startX = pos.x() - (handSize * 15 + (cardW - 15)) / 2;
            for (int c = 0; c < handSize; ++c) {
                drawUnoCard(p, QRect(startX + c * 15, pos.y() + 25, cardW - 25, cardH - 35), nullptr, false);
            }
        }
    }
}

// ============================================================================
// MAIN WINDOW
// ============================================================================

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(getLocalizedText("Royal Card Club Collection", "Royal Card Club Collection"));
#if defined(Q_OS_IOS)
    showFullScreen();
#else
    resize(1050, 750);
    setMinimumSize(950, 680);
#endif

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
    setCentralWidget(stackedWidget);

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
                    QString act = json["action"].toString();
                    int amt = json["amount"].toInt(0);
                    pokerWidget->engine.processAction(senderId, act, amt);
                    pokerWidget->broadcastNetState();
                    pokerWidget->updateUI();
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
            .arg(PokerConfig::MAX_PLAYERS);

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

        AudioManager::instance().startMusic();
}

MainWindow::~MainWindow() {
    delete netManager;
}
