#include <QNetworkInterface>

#include "NetworkManager.h"
#include "AppSettings.h"

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {}

NetworkManager::~NetworkManager() {
    disconnectAll();
}

void NetworkManager::disconnectAll() {
    stopDiscoveryBroadcast();
    stopDiscoveryListening();

    // Безопасная остановка и удаление таймера таймаута
    if (connectionTimeoutTimer) {
        connectionTimeoutTimer->stop();
        connectionTimeoutTimer->deleteLater();
    }

    isHost = false;
    isNetworkGame = false;
    isLobby = false;
    isSessionActive = false;
    myIdx = 0;

    if (tcpServer) {
        tcpServer->close();
        tcpServer->deleteLater(); // Безопасное отложенное удаление вместо delete
        tcpServer = nullptr;
    }

    if (tcpSocket) {
        tcpSocket->abort();
        tcpSocket->deleteLater(); // Безопасное отложенное удаление
        tcpSocket = nullptr;
    }

    for (auto* s : clientSockets) {
        if (s) {
            s->abort();
            s->deleteLater();
        }
    }
    clientSockets.clear();
    lobbyClients.clear();
}

// =============================================================================
// UDP BROADCAST & LAN DISCOVERY
// =============================================================================

void NetworkManager::startDiscoveryBroadcast() {
    stopDiscoveryBroadcast();

    udpBeaconSocket = new QUdpSocket(this);
    udpBeaconTimer  = new QTimer(this);

    connect(udpBeaconTimer, &QTimer::timeout, this, &NetworkManager::sendDiscoveryBeacon);
    udpBeaconTimer->start(1000); // Вещание 1 раз в секунду
    sendDiscoveryBeacon();
}

void NetworkManager::stopDiscoveryBroadcast() {
    if (udpBeaconTimer) {
        udpBeaconTimer->stop();
        delete udpBeaconTimer;
        udpBeaconTimer = nullptr;
    }
    if (udpBeaconSocket) {
        udpBeaconSocket->close();
        delete udpBeaconSocket;
        udpBeaconSocket = nullptr;
    }
}

void NetworkManager::sendDiscoveryBeacon() {
    if (!udpBeaconSocket || !isHost || !isLobby) return;

    QJsonObject beacon;
    beacon["tag"]         = "RCC_BEACON";
    beacon["hostName"]    = AppSettings::instance().getNickname();
    beacon["gameType"]    = gameType;
    beacon["playerCount"] = getActiveClientCount() + 1;
    beacon["maxPlayers"]  = NetConfig::MAX_PLAYERS;
    beacon["port"]        = AppSettings::instance().getServerPort();

    const QByteArray datagram = QJsonDocument(beacon).toJson(QJsonDocument::Compact);

    // Стандартный общий Broadcast (255.255.255.255)
    udpBeaconSocket->writeDatagram(datagram, QHostAddress::Broadcast, NetConfig::DISCOVERY_PORT);

    // Точный Broadcast по всем активным Wi-Fi интерфейсам (критично для iOS и macOS)
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const auto& iface : interfaces) {
        // Проверяем, что интерфейс включен, работает, поддерживает Broadcast и не является локальной петлей (Loopback)
        if (iface.flags().testFlag(QNetworkInterface::IsUp) && iface.flags().testFlag(QNetworkInterface::IsRunning) && iface.flags().testFlag(QNetworkInterface::CanBroadcast) && !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            for (const auto& entry : iface.addressEntries()) {
                const QHostAddress bcastAddr = entry.broadcast();
                // Отправляем на прямой широковещательный адрес подсети (например, 192.168.0.255)
                if (!bcastAddr.isNull() && bcastAddr != QHostAddress::Broadcast) {
                    udpBeaconSocket->writeDatagram(datagram, bcastAddr, NetConfig::DISCOVERY_PORT);
                }
            }
        }
    }
}

void NetworkManager::startDiscoveryListening() {
    stopDiscoveryListening();

    discoveredLobbies.clear();
    udpDiscoverySocket = new QUdpSocket(this);

    // Привязка сокета с флагами совместного использования порта
    udpDiscoverySocket->bind(QHostAddress::AnyIPv4, NetConfig::DISCOVERY_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(udpDiscoverySocket, &QUdpSocket::readyRead, this, &NetworkManager::onDiscoveryDatagramReceived);

    // Таймер очистки неактивных серверов (не отвечавших более 3.5 секунд)
    lobbyCleanupTimer = new QTimer(this);
    connect(lobbyCleanupTimer, &QTimer::timeout, this, &NetworkManager::cleanupStaleLobbies);
    lobbyCleanupTimer->start(1500);
}

void NetworkManager::stopDiscoveryListening() {
    if (lobbyCleanupTimer) {
        lobbyCleanupTimer->stop();
        delete lobbyCleanupTimer;
        lobbyCleanupTimer = nullptr;
    }
    if (udpDiscoverySocket) {
        udpDiscoverySocket->close();
        delete udpDiscoverySocket;
        udpDiscoverySocket = nullptr;
    }
    discoveredLobbies.clear();
}

void NetworkManager::onDiscoveryDatagramReceived() {
    while (udpDiscoverySocket && udpDiscoverySocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(udpDiscoverySocket->pendingDatagramSize()));
        QHostAddress senderAddr;
        quint16 senderPort;

        udpDiscoverySocket->readDatagram(datagram.data(), datagram.size(), &senderAddr, &senderPort);

        const QJsonDocument doc = QJsonDocument::fromJson(datagram);
        if (!doc.isObject()) continue;
        const QJsonObject json = doc.object();

        if (json["tag"].toString() != "RCC_BEACON") continue;

        // Преобразование адреса IPv4-mapped IPv6 в стандартный IPv4 вид
        QString hostIp = senderAddr.toString();
        if (senderAddr.protocol() == QAbstractSocket::IPv6Protocol) {
            bool ok = false;
            const QHostAddress ipv4(senderAddr.toIPv4Address(&ok));
            if (ok) hostIp = ipv4.toString();
        }

        const quint16 tcpPort = static_cast<quint16>(json["port"].toInt(12345));
        const QString hostName = json["hostName"].toString();
        const int gType = json["gameType"].toInt();
        const int pCount = json["playerCount"].toInt();
        const int maxP = json["maxPlayers"].toInt(NetConfig::MAX_PLAYERS);
        const qint64 now = QDateTime::currentMSecsSinceEpoch();

        // Поиск и обновление существующего лобби или добавление нового
        bool found = false;
        for (auto& lobby : discoveredLobbies) {
            if (lobby.ip == hostIp && lobby.port == tcpPort) {
                lobby.hostName    = hostName;
                lobby.gameType    = gType;
                lobby.playerCount = pCount;
                lobby.maxPlayers  = maxP;
                lobby.lastSeenMs  = now;
                found = true;
                break;
            }
        }

        if (!found) {
            DiscoveredLobby newLobby;
            newLobby.ip          = hostIp;
            newLobby.port        = tcpPort;
            newLobby.hostName    = hostName;
            newLobby.gameType    = gType;
            newLobby.playerCount = pCount;
            newLobby.maxPlayers  = maxP;
            newLobby.lastSeenMs  = now;
            discoveredLobbies.append(newLobby);
        }

        emit lobbiesUpdated(discoveredLobbies);
    }
}

void NetworkManager::cleanupStaleLobbies() {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const int initialSize = discoveredLobbies.size();

    for (int i = discoveredLobbies.size() - 1; i >= 0; --i) {
        if (now - discoveredLobbies[i].lastSeenMs > 3500) {
            discoveredLobbies.removeAt(i);
        }
    }

    if (discoveredLobbies.size() != initialSize) {
        emit lobbiesUpdated(discoveredLobbies);
    }
}

int NetworkManager::getActiveClientCount() const {
    int count = 0;
    for (auto* s : clientSockets) {
        if (s && s->state() == QAbstractSocket::ConnectedState) {
            count++;
        }
    }
    return count;
}

void NetworkManager::startHostServer(int selectedGameType) {
    disconnectAll();
    isHost = true;
    isNetworkGame = true;
    isLobby = true;
    myIdx = 0;
    gameType = selectedGameType;

    ConnectedClient host;
    host.id = 0;
    host.name = AppSettings::instance().getNickname();
    host.avatar = static_cast<int>(AppSettings::instance().getAvatar());
    lobbyClients.append(host);

    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, [this]() {
        QTcpSocket* socket = tcpServer->nextPendingConnection();

        // Поиск освободившегося слота после отключения
        int slotIdx = -1;
        for (int i = 0; i < clientSockets.size(); ++i) {
            if (clientSockets[i] == nullptr) {
                slotIdx = i;
                break;
            }
        }

        if (slotIdx != -1) {
            clientSockets[slotIdx] = socket;
        } else {
            if (clientSockets.size() >= NetConfig::MAX_CLIENTS) {
                socket->abort();
                socket->deleteLater();
                return;
            }
            clientSockets.append(socket);
        }

        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onNetworkReadHost);
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            const int idx = clientSockets.indexOf(socket);
            if (idx != -1) {
                if (isLobby) {
                    clientSockets.removeAt(idx);
                    if (idx + 1 < lobbyClients.size()) lobbyClients.removeAt(idx + 1);

                    QJsonObject lobbyJson;
                    lobbyJson["isLobby"]     = isLobby;
                    lobbyJson["gameType"]    = gameType;
                    lobbyJson["playerCount"] = getActiveClientCount() + 1;
                    broadcastJson(lobbyJson);

                    emit lobbyStatusChanged(QString(getLocalizedText("ЛОББИ: %1/%2 игроков. Ожидание...", "LOBBY: %1/%2 players. Waiting..."))
                    .arg(getActiveClientCount() + 1)
                    .arg(NetConfig::MAX_PLAYERS));
                } else {
                    clientSockets[idx] = nullptr;
                    if (idx + 1 < lobbyClients.size()) lobbyClients[idx + 1].isDisconnected = true;
                    emit signalPlayerDisconnected(idx + 1);
                }
            }
            socket->disconnect();
            socket->abort();
            socket->deleteLater();
        });
    });

    if (!tcpServer->listen(QHostAddress::Any, AppSettings::instance().getServerPort())) {
        emit lobbyStatusChanged(getLocalizedText("Ошибка: не удалось запустить сервер!", "Error: Failed to start server!"));
    } else {
        startDiscoveryBroadcast(); // Запуск периодического вещания маяков в сеть
    }
}
void NetworkManager::connectToHost(const QString& ip, int mySelectedGameType, quint16 port) {
    disconnectAll();
    isHost = false;
    isNetworkGame = true;
    isLobby = true;
    myIdx = 1;
    selectedClientGameType = mySelectedGameType;
    isSessionActive = false;

    const quint16 targetPort = (port != 0) ? port : AppSettings::instance().getServerPort();

    emit lobbyStatusChanged(getLocalizedText("Подключение к серверу...", "Connecting to server..."));

    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &NetworkManager::onNetworkReadClient);

    // Создаем таймер через QPointer
    connectionTimeoutTimer = new QTimer(this);
    connectionTimeoutTimer->setSingleShot(true);

    connect(tcpSocket, &QTcpSocket::connected, this, [this]() {
        if (connectionTimeoutTimer) {
            connectionTimeoutTimer->stop();
            connectionTimeoutTimer->deleteLater();
        }

        emit lobbyStatusChanged(getLocalizedText("Подключено! Ожидание лобби...", "Connected! Waiting for lobby..."));

        QJsonObject joinJson;
        joinJson["action"] = "JOIN";
        joinJson["name"]   = AppSettings::instance().getNickname();
        joinJson["avatar"] = static_cast<int>(AppSettings::instance().getAvatar());
        sendJsonToServer(joinJson);
    });

    auto handleDisconnect = [this]() {
        if (connectionTimeoutTimer) {
            connectionTimeoutTimer->stop();
            connectionTimeoutTimer->deleteLater();
        }

        // Если разрыв уже был обработан - выходим
        if (!isNetworkGame && !tcpSocket) return;

        const bool wasInSession = isSessionActive;
        const QString msg = wasInSession ? getLocalizedText("Связь с сервером потеряна! Хост отключился.", "Connection lost! Host disconnected.") : getLocalizedText("Ошибка: Сервер не найден или лобби не существует!", "Error: Server not found or lobby does not exist!");

        disconnectAll();
        emit lobbyStatusChanged(msg);
        if (wasInSession) {
            emit signalHostDisconnected();
        }
    };

    connect(connectionTimeoutTimer.data(), &QTimer::timeout, this, [this, handleDisconnect]() {
        if (tcpSocket && tcpSocket->state() != QAbstractSocket::ConnectedState) {
            handleDisconnect();
        }
    });

    connect(tcpSocket, &QAbstractSocket::errorOccurred, this, [handleDisconnect](QAbstractSocket::SocketError) {
        handleDisconnect();
    }, Qt::QueuedConnection);

    connect(tcpSocket, &QTcpSocket::disconnected, this, [handleDisconnect]() {
        handleDisconnect();
    }, Qt::QueuedConnection);

    connectionTimeoutTimer->start(5000);
    tcpSocket->connectToHost(ip, targetPort);
}

void NetworkManager::broadcastJson(const QJsonObject& json) {
    if (!isHost) return;
    const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact) + "\n";
    for (auto* socket : clientSockets) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(data);
        }
    }
}

void NetworkManager::sendJsonToClient(int clientIdx, const QJsonObject& json) {
    if (!isHost || clientIdx < 0 || clientIdx >= clientSockets.size()) return;
    auto* socket = clientSockets[clientIdx];
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(QJsonDocument(json).toJson(QJsonDocument::Compact) + "\n");
        socket->flush();
    }
}

void NetworkManager::sendJsonToServer(const QJsonObject& json) {
    if (tcpSocket && tcpSocket->state() == QAbstractSocket::ConnectedState) {
        tcpSocket->write(QJsonDocument(json).toJson(QJsonDocument::Compact) + "\n");
        tcpSocket->flush();
    }
}

void NetworkManager::onNetworkReadHost() {
    auto* senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;

    while (senderSocket->canReadLine()) {
        const QByteArray line = senderSocket->readLine();
        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;
        const QJsonObject json = doc.object();

        if (json["action"].toString() == "JOIN") {
            QString cName = json["name"].toString().trimmed();
            const int cAvatar = json["avatar"].toInt(0);
            if (cName.isEmpty()) cName = getLocalizedText("Игрок", "Player");

            // Проверяем совпадение с именем хоста
            if (!lobbyClients.isEmpty() && lobbyClients[0].name == cName) {
                QJsonObject errJson;
                errJson["error"] = true;
                errJson["message"] = getLocalizedText("Ошибка: Игрок с таким именем уже в лобби!", "Error: Player with this name is already in lobby!");
                senderSocket->write(QJsonDocument(errJson).toJson(QJsonDocument::Compact) + "\n");
                senderSocket->flush();
                senderSocket->disconnectFromHost();
                return;
            }

            // Ищем существующего участника с таким же ником
            int existingIdx = -1;
            for (int i = 1; i < lobbyClients.size(); ++i) {
                if (lobbyClients[i].name == cName) {
                    existingIdx = i;
                    break;
                }
            }

            // Отклоняем только если игрок с таким именем УЖЕ АКТИВЕН И ПОДКЛЮЧЕН в данный момент
            if (existingIdx != -1 && !lobbyClients[existingIdx].isDisconnected) {
                QJsonObject errJson;
                errJson["error"] = true;
                errJson["message"] = getLocalizedText("Ошибка: Игрок с таким именем уже в лобби!", "Error: Player with this name is already in lobby!");
                senderSocket->write(QJsonDocument(errJson).toJson(QJsonDocument::Compact) + "\n");
                senderSocket->flush();
                senderSocket->disconnectFromHost();
                return;
            }

            // Если мы находимся в лобби, а в списке осталась старая отключенная запись - очищаем её
            if (isLobby && existingIdx != -1 && lobbyClients[existingIdx].isDisconnected) {
                lobbyClients.removeAt(existingIdx);
                existingIdx = -1;
            }

            // Подключение / переподключение игрока
            if (existingIdx > 0) {
                const int socketSlot = existingIdx - 1;
                if (socketSlot < clientSockets.size()) {
                    auto* oldSocket = clientSockets[socketSlot];
                    if (oldSocket && oldSocket != senderSocket) {
                        oldSocket->abort();
                        oldSocket->deleteLater();
                    }
                    clientSockets[socketSlot] = senderSocket;
                }
                lobbyClients[existingIdx].isDisconnected = false;
                lobbyClients[existingIdx].avatar = cAvatar;
                emit signalPlayerReconnected(existingIdx);
            } else {
                int clientIdx = clientSockets.indexOf(senderSocket);
                if (clientIdx == -1) {
                    clientSockets.append(senderSocket);
                    clientIdx = clientSockets.size() - 1;
                }
                const int playerId = clientIdx + 1;
                while (lobbyClients.size() <= playerId) {
                    ConnectedClient cl;
                    cl.id = lobbyClients.size();
                    cl.name = QString(getLocalizedText("Игрок %1", "Player %1")).arg(cl.id + 1);
                    lobbyClients.append(cl);
                }
                lobbyClients[playerId].name = cName;
                lobbyClients[playerId].avatar = cAvatar;
                lobbyClients[playerId].isDisconnected = false;
            }

            QJsonObject lobbyJson;
            lobbyJson["isLobby"]     = isLobby;
            lobbyJson["gameType"]    = gameType;
            lobbyJson["playerCount"] = getActiveClientCount() + 1;
            broadcastJson(lobbyJson);

            if (isLobby) {
                emit lobbyStatusChanged(QString(getLocalizedText("ЛОББИ: %1/%2 игроков. Ожидание...", "LOBBY: %1/%2 players. Waiting..."))
                .arg(getActiveClientCount() + 1)
                .arg(NetConfig::MAX_PLAYERS));
            }

            if (!isLobby) {
                emit signalNetworkDataReceived(existingIdx != -1 ? existingIdx : clientSockets.indexOf(senderSocket) + 1, json);
            }
            continue;
        }

        const int clientIdx = clientSockets.indexOf(senderSocket);
        if (clientIdx == -1) continue;
        emit signalNetworkDataReceived(clientIdx + 1, json);
    }
}

void NetworkManager::onNetworkReadClient() {
    while (tcpSocket && tcpSocket->canReadLine()) {
        const QByteArray line = tcpSocket->readLine();
        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;
        const QJsonObject json = doc.object();

        isSessionActive = true;

        if (json.contains("error") && json["error"].toBool()) {
            const QString errMsg = json["message"].toString();
            isLobby = false;
            disconnectAll();
            emit lobbyStatusChanged(errMsg);
            return;
        }

        const int serverGameType = json["gameType"].toInt();
        if (serverGameType != selectedClientGameType) {
            static const QString gameNames[] = {
                getLocalizedText("Покера", "Poker"),
                getLocalizedText("Дурака", "Durak"),
                getLocalizedText("Козла", "Kozel"),
                getLocalizedText("Уно", "UNO")
            };
            const QString myGame = gameNames[std::clamp(selectedClientGameType, 0, 3)];
            const QString serverGame = gameNames[std::clamp(serverGameType, 0, 3)];

            const QString errorMsg = QString(getLocalizedText("Ошибка: Вы выбрали %1, а это лобби %2!", "Error: You selected %1, but this is a %2 lobby!"))
            .arg(myGame, serverGame);

            isLobby = false;
            disconnectAll();
            emit lobbyStatusChanged(errorMsg);
            return;
        }

        isLobby = json["isLobby"].toBool();
        if (isLobby) {
            const int count = json["playerCount"].toInt();
            static const QString gameNames[] = {
                getLocalizedText("ПОКЕРА", "POKER"),
                getLocalizedText("ДУРАКА", "DURAK"),
                getLocalizedText("КОЗЛА", "KOZEL"),
                getLocalizedText("УНО", "UNO")
            };
            emit lobbyStatusChanged(QString(getLocalizedText("ЛОББИ (%1): %2/4 игроков. Ожидание старта...", "LOBBY (%1): %2/4 players. Waiting to start..."))
            .arg(gameNames[serverGameType])
            .arg(count));
            continue;
        }

        if (json.contains("yourId")) {
            myIdx = json["yourId"].toInt();
        }

        emit signalClientGameStarted(serverGameType, json);
        emit signalNetworkDataReceived(0, json);
    }
}
