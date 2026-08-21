#include "dialogs/LanLobbiesDialog.h"
#include "AppSettings.h"
#include "Audio.h"

#include <algorithm>

LanLobbiesDialog::LanLobbiesDialog(NetworkManager* netMgr, QWidget* parent)
: QDialog(parent ? parent->window() : nullptr), m_netManager(netMgr)
{
    setWindowTitle(getLocalizedText("Поиск лобби в сети", "LAN Lobby Discovery"));

    #if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setModal(true);
    #endif

    QWidget* topWin = parent ? parent->window() : nullptr;
    const int winW = topWin ? topWin->width() : (parent ? parent->width() : 1280);
    const int winH = topWin ? topWin->height() : (parent ? parent->height() : 720);
    const qreal s = std::clamp(std::min(winW / 1280.0, winH / 720.0), 0.7, 1.4);
    const int maxW = qMin(qRound(winW * 0.92), qRound(560 * s));
    const int maxH = qMin(qRound(winH * 0.90), qRound(420 * s));
    setGeometry((winW - maxW) / 2, (winH - maxH) / 2, maxW, maxH);

    const int fTitle = qMax(16, qRound(20 * s));
    const int fBase  = qMax(11, qRound(13 * s));
    const int pad    = qMax(5, qRound(8 * s));

    setStyleSheet(QString(R"(
        QDialog { background-color: #0B1120; border: 1px solid rgba(251, 191, 36, 0.35); border-radius: %1px; }
        QLabel { color: #E2E8F0; font-size: %2px; font-weight: bold; }
        QListWidget { background: #0F172A; border: 1px solid rgba(251, 191, 36, 0.2); border-radius: 8px; color: #F8FAFC; font-size: %2px; padding: %3px; }
        QListWidget::item { padding: %3px; border-radius: 6px; margin-bottom: 4px; background: #1E293B; }
        QListWidget::item:hover { background: #334155; }
        QListWidget::item:selected { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1D4ED8, stop:1 #3B82F6); color: white; }
        QScrollBar:vertical { background: #0F172A; width: 6px; border-radius: 3px; }
        QScrollBar::handle:vertical { background: #334155; border-radius: 3px; }
    )").arg(qRound(12 * s)).arg(fBase).arg(pad));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(qRound(16 * s), qRound(16 * s), qRound(16 * s), qRound(16 * s));
    mainLayout->setSpacing(qRound(10 * s));

    // Заголовок диалога
    auto* lblTitle = new QLabel(getLocalizedText("📡 НАЙДЕННЫЕ СЕРВЕРЫ В СЕТИ", "📡 ACTIVE LAN SERVERS"), this);
    lblTitle->setStyleSheet(QString("color: #FBBF24; font-size: %1px; font-weight: 900; letter-spacing: 1px;").arg(fTitle));
    lblTitle->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(lblTitle);

    // Статусная строка сканирования
    m_lblStatus = new QLabel(getLocalizedText("Сканирование локальной сети...", "Scanning local network..."), this);
    m_lblStatus->setStyleSheet("color: #94A3B8; font-style: italic;");
    m_lblStatus->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_lblStatus);

    // Список найденных комнат
    m_listWidget = new QListWidget(this);
    mainLayout->addWidget(m_listWidget);

    // Кнопки управления
    auto* btnLayout = new QHBoxLayout();
    m_btnCancel = new QPushButton(getLocalizedText("Отмена", "Cancel"), this);
    m_btnCancel->setCursor(Qt::PointingHandCursor);
    m_btnCancel->setStyleSheet(QString(R"(
        QPushButton { background: #334155; color: #F8FAFC; font-weight: bold; font-size: %1px; padding: %2px; border-radius: 8px; }
        QPushButton:hover { background: #475569; }
    )").arg(fBase).arg(qRound(8 * s)));
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    m_btnConnect = new QPushButton(getLocalizedText("Войти в лобби", "Join Lobby"), this);
    m_btnConnect->setCursor(Qt::PointingHandCursor);
    m_btnConnect->setEnabled(false);
    m_btnConnect->setStyleSheet(QString(R"(
        QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #059669, stop:1 #10B981); color: white; font-weight: bold; font-size: %1px; padding: %2px; border-radius: 8px; }
        QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #10B981, stop:1 #34D399); }
        QPushButton:disabled { background: #1E293B; color: #64748B; }
    )").arg(fBase).arg(qRound(8 * s)));
    connect(m_btnConnect, &QPushButton::clicked, this, &LanLobbiesDialog::onConnectClicked);

    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnConnect);
    mainLayout->addLayout(btnLayout);

    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        m_btnConnect->setEnabled(m_listWidget->currentRow() >= 0);
    });

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this]() {
        onConnectClicked();
    });

    // Подключение к сигналам сетевого менеджера
    if (m_netManager) {
        connect(m_netManager, &NetworkManager::lobbiesUpdated, this, &LanLobbiesDialog::updateLobbiesList);
        m_netManager->startDiscoveryListening();
    }
}

LanLobbiesDialog::~LanLobbiesDialog() {
    if (m_netManager) {
        m_netManager->stopDiscoveryListening();
    }
}

void LanLobbiesDialog::updateLobbiesList(const QVector<DiscoveredLobby>& lobbies) {
    m_lobbies = lobbies;
    const int prevRow = m_listWidget->currentRow();
    m_listWidget->clear();

    static const QString gameNames[] = {
        getLocalizedText("Покер", "Poker"),
        getLocalizedText("Дурак", "Durak"),
        getLocalizedText("Козёл", "Kozel"),
        getLocalizedText("Уно", "UNO")
    };

    for (const auto& lobby : m_lobbies) {
        const QString gName = (lobby.gameType >= 0 && lobby.gameType < 4) ? gameNames[lobby.gameType] : "Unknown";
        const QString itemText = QString("%1 | %2 (%3/%4) | IP: %5:%6")
        .arg(lobby.hostName)
        .arg(gName)
        .arg(lobby.playerCount)
        .arg(lobby.maxPlayers)
        .arg(lobby.ip)
        .arg(lobby.port);

        m_listWidget->addItem(itemText);
    }

    if (m_lobbies.isEmpty()) {
        m_lblStatus->setText(getLocalizedText("Серверы не найдены. Ожидание вещания...", "No servers found. Waiting for broadcast..."));
        m_btnConnect->setEnabled(false);
    } else {
        m_lblStatus->setText(QString(getLocalizedText("Найдено серверов: %1", "Found servers: %1")).arg(m_lobbies.size()));
        if (prevRow >= 0 && prevRow < m_listWidget->count()) {
            m_listWidget->setCurrentRow(prevRow);
        }
    }
}

void LanLobbiesDialog::onConnectClicked() {
    const int row = m_listWidget->currentRow();
    if (row >= 0 && row < m_lobbies.size()) {
        AudioManager::instance().playSound(SoundEffect::ButtonClick);
        m_selectedLobby = m_lobbies[row];
        accept();
    }
}
