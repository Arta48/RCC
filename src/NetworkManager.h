#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>

namespace NetConfig {
    constexpr int MAX_PLAYERS = 4;
    constexpr int MAX_CLIENTS = MAX_PLAYERS - 1;
}

/**
 * @brief Метаданные подключенного клиента в лобби.
 */
struct ConnectedClient {
    int     id = 0;
    QString name;
    int     avatar = 0;
    bool    isDisconnected = false;
};

/**
 * @brief Сетевой менеджер (Host / Client) для координации комнат и передачи JSON-сообщений.
 * Полностью изолирован от правил конкретных игр.
 */
class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager() override;

    bool isHost           = true;
    bool isNetworkGame    = false;
    bool isLobby          = true;
    bool isSessionActive  = false;
    int  myIdx            = 0;

    int gameType               = 0; // 0: Покер, 1: Дурак, 2: Козёл, 3: Уно
    int selectedClientGameType = 0;

    QTcpServer*              tcpServer = nullptr;
    QTcpSocket*              tcpSocket = nullptr;
    QVector<QTcpSocket*>     clientSockets;
    QVector<ConnectedClient> lobbyClients;

    void startHostServer(int selectedGameType);
    void connectToHost(const QString& ip, int mySelectedGameType);
    void disconnectAll();

    void broadcastJson(const QJsonObject& json);
    void sendJsonToClient(int clientIdx, const QJsonObject& json);
    void sendJsonToServer(const QJsonObject& json);

    int getActiveClientCount() const;

signals:
    void lobbyStatusChanged(const QString& message);
    void signalStartNetworkGame(int gameType, int activeClients);
    void signalClientGameStarted(int gameType, const QJsonObject& state);
    void signalHostDisconnected();
    void signalPlayerReconnected(int pIdx);
    void signalPlayerDisconnected(int pIdx);
    void signalNetworkDataReceived(int senderId, const QJsonObject& json);

private slots:
    void onNetworkReadHost();
    void onNetworkReadClient();
};
