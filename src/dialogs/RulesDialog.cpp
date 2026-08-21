#include "RulesDialog.h"
#include "AppSettings.h"

#include <QWindow>
#include <algorithm>

RulesDialog::RulesDialog(int defaultTabIndex, QWidget* parent)
: QDialog(parent ? parent->window() : nullptr)
{
    setWindowTitle(getLocalizedText("Правила игры", "Game Rules"));

    #if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setModal(true);
    #endif

    QWidget* topWin = parent ? parent->window() : nullptr;
    const int winW = topWin ? topWin->width() : (parent ? parent->width() : 1280);
    const int winH = topWin ? topWin->height() : (parent ? parent->height() : 720);
    const qreal s = std::clamp(std::min(winW / 1280.0, winH / 720.0), 0.7, 1.4);
    const int maxW = qMin(qRound(winW * 0.92), qRound(660 * s));
    const int maxH = qMin(qRound(winH * 0.90), qRound(480 * s));
    setGeometry((winW - maxW) / 2, (winH - maxH) / 2, maxW, maxH);

    const int fTitle = qMax(16, qRound(20 * s));
    const int fBase  = qMax(12, qRound(13 * s));
    const int pad    = qMax(5, qRound(8 * s));

    #if defined(Q_OS_ANDROID)
    const QString dlgBg = "#0B1120";
    const QString paneBg = "#0F172A";
    const QString textBg = "#0F172A";
    #else
    const QString dlgBg = "rgba(11, 17, 32, 0.96)";
    const QString paneBg = "rgba(15, 23, 42, 0.90)";
    const QString textBg = "transparent";
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

    const QString pokerRules = getLocalizedText(
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

    const QString durakRules = getLocalizedText(
        "<h3>Дурак (Подкидной)</h3>"
        "<p><b>Колода:</b> 36 карт (от 6 до Туза). Игрокам раздаётся по 6 карт, нижняя карта колоды открывается и определяет козырь.</p>"
        "<p><b>Цель:</b> Первым сбросить все карты.</p>"
        "<p><b>Ход игры:</b> Игрок с младшим козырем ходит первым. Атаковать можно картами одного достоинства. Отбивающийся бьёт карты старшей картой той же масти или козырем.</p>"
        "<p><b>Подкидывание:</b> Атакующий и другие игроки могут подкидывать карты совпадающих достоинств (не более количества карт у отбивающегося и не более 6).</p>"
        "<p><b>Добор:</b> После отбоя (или взятия) игроки по очереди добирают до 6 карт: сначала атакующий, затем подкидывавшие, последним - защитник.</p>",
        "<h3>Durak (Podkidnoy)</h3>"
        "<p><b>Deck:</b> 36 cards (6 to Ace). Players receive 6 cards each. The bottom card is revealed to set the trump suit.</p>"
        "<p><b>Goal:</b> Shed all cards first.</p>"
        "<p><b>Gameplay:</b> Lowest trump leads first. Attack with matching ranks. Defender beats attacks with a higher card of the same suit or trump.</p>"
        "<p><b>Tossing:</b> Players can toss additional cards matching any rank on the table (up to defender's hand size, max 6 per bout).</p>"
        "<p><b>Drawing:</b> After a bout (Done / Taken), players replenish back up to 6 cards in clockwise order: attacker first, tossers next, defender last.</p>"
    );

    const QString kozelRules = getLocalizedText(
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

    const QString unoRules = getLocalizedText(
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

void RulesDialog::showEvent(QShowEvent* event) {
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

void RulesDialog::hideEvent(QHideEvent* event) {
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
