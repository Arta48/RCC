#include "NetworkManager.h"
#include "AppSettings.h"

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {}

NetworkManager::~NetworkManager() {
    disconnectAll();
}

void NetworkManager::disconnectAll() {
    isHost = false;
    isNetworkGame = false;
    isLobby = false;
    isSessionActive = false;
    myIdx = 0;

    if (tcpServer) {
        tcpServer->close();
        delete tcpServer;
        tcpServer = nullptr;
    }

    if (tcpSocket) {
        tcpSocket->disconnect();
        tcpSocket->abort();
        tcpSocket->deleteLater();
        tcpSocket = nullptr;
    }

    for (auto* s : clientSockets) {
        if (s) {
            s->disconnect();
            s->abort();
            s->deleteLater();
        }
    }
    clientSockets.clear();
    lobbyClients.clear();
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
    host.name = AppSettings::instance().nickname;
    host.avatar = static_cast<int>(AppSettings::instance().avatar);
    lobbyClients.append(host);

    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, [this]() {
        QTcpSocket* socket = tcpServer->nextPendingConnection();

        // Поиск освободившегося слота для повторного подключения
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
            int idx = clientSockets.indexOf(socket);
            if (idx != -1) {
                if (isLobby) {
                    clientSockets.removeAt(idx);
                    if (idx + 1 < lobbyClients.size()) lobbyClients.removeAt(idx + 1);

                    // Оповещаем оставшихся клиентов
                    QJsonObject lobbyJson;
                    lobbyJson["isLobby"]     = isLobby;
                    lobbyJson["gameType"]    = gameType;
                    lobbyJson["playerCount"] = getActiveClientCount() + 1;
                    broadcastJson(lobbyJson);

                    // Обновляем статус у хоста при выходе игрока из лобби
                    emit lobbyStatusChanged(QString(getLocalizedText("ЛОББИ: %1/%2 игроков. Ожидание...", "LOBBY: %1/%2 players. Waiting...")).arg(getActiveClientCount() + 1).arg(NetConfig::MAX_PLAYERS));
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

    if (!tcpServer->listen(QHostAddress::Any, AppSettings::instance().serverPort)) {
        emit lobbyStatusChanged(getLocalizedText("Ошибка: не удалось запустить сервер!", "Error: Failed to start server!"));
    }
}

void NetworkManager::connectToHost(const QString& ip, int mySelectedGameType) {
    disconnectAll();
    isHost = false;
    isNetworkGame = true;
    isLobby = true;
    myIdx = 1;
    selectedClientGameType = mySelectedGameType;

    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &NetworkManager::onNetworkReadClient);

    isSessionActive = false;

    connect(tcpSocket, &QTcpSocket::connected, this, [this]() {
        emit lobbyStatusChanged(getLocalizedText("Подключено! Ожидание лобби...", "Connected! Waiting for lobby..."));

        QJsonObject joinJson;
        joinJson["action"] = "JOIN";
        joinJson["name"]   = AppSettings::instance().nickname;
        joinJson["avatar"] = static_cast<int>(AppSettings::instance().avatar);
        sendJsonToServer(joinJson);
    });

    auto handleDisconnect = [this]() {
        if (!isNetworkGame || isHost) return; // Игнорируем вызов, если сеть уже сброшена

        bool wasInSession = isSessionActive;
        QString msg = wasInSession ? getLocalizedText("Связь с сервером потеряна! Хост отключился.", "Connection lost! Host disconnected.") : getLocalizedText("Ошибка: Сервер не найден или лобби не существует!", "Error: Server not found or lobby does not exist!");

        isLobby = false;
        disconnectAll();
        emit lobbyStatusChanged(msg);
        if (wasInSession) {
            emit signalHostDisconnected();
        }
    };

    connect(tcpSocket, &QAbstractSocket::errorOccurred, this, [handleDisconnect](QAbstractSocket::SocketError) {
        handleDisconnect();
    });

    connect(tcpSocket, &QTcpSocket::disconnected, this, [handleDisconnect]() {
        handleDisconnect();
    });

    tcpSocket->connectToHost(ip, AppSettings::instance().serverPort);
}

void NetworkManager::broadcastJson(const QJsonObject& json) {
    if (!isHost) return;
    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact) + "\n";
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
        QByteArray line = senderSocket->readLine();
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;
        QJsonObject json = doc.object();

        if (json["action"].toString() == "JOIN") {
            QString cName = json["name"].toString().trimmed();
            int cAvatar = json["avatar"].toInt(0);
            if (cName.isEmpty()) cName = getLocalizedText("Игрок", "Player");

            // Проверяем всех участников
            int existingIdx = -1;
            for (int i = 0; i < lobbyClients.size(); ++i) {
                if (lobbyClients[i].name == cName) {
                    existingIdx = i;
                    break;
                }
            }

            // Если имя совпало с хостом или с уже подключенным игроком — отклоняем новичка
            if (existingIdx == 0 || (existingIdx != -1 && (isLobby || !lobbyClients[existingIdx].isDisconnected))) {
                QJsonObject errJson;
                errJson["error"] = true;
                errJson["message"] = getLocalizedText("Ошибка: Игрок с таким именем уже в лобби!", "Error: Player with this name is already in lobby!");
                senderSocket->write(QJsonDocument(errJson).toJson(QJsonDocument::Compact) + "\n");
                senderSocket->flush();
                senderSocket->disconnectFromHost();
                return;
            }

            // Легитимный реконнект отключившегося клиента посреди матча
            if (existingIdx > 0) {
                int socketSlot = existingIdx - 1;
                if (socketSlot < clientSockets.size()) {
                    auto* oldSocket = clientSockets[socketSlot];
                    if (oldSocket && oldSocket != senderSocket) {
                        oldSocket->abort();
                        oldSocket->disconnect();
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
                int playerId = clientIdx + 1;
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
                emit lobbyStatusChanged(QString(getLocalizedText("ЛОББИ: %1/%2 игроков. Ожидание...", "LOBBY: %1/%2 players. Waiting...")).arg(getActiveClientCount() + 1).arg(NetConfig::MAX_PLAYERS));
            }

            if (!isLobby) {
                emit signalNetworkDataReceived(existingIdx != -1 ? existingIdx : clientSockets.indexOf(senderSocket) + 1, json);
            }
            continue;
        }

        int clientIdx = clientSockets.indexOf(senderSocket);
        if (clientIdx == -1) continue;
        emit signalNetworkDataReceived(clientIdx + 1, json);
    }
}

void NetworkManager::onNetworkReadClient() {
    while (tcpSocket && tcpSocket->canReadLine()) {
        QByteArray line = tcpSocket->readLine();
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;
        QJsonObject json = doc.object();

        isSessionActive = true;

        // Обработка ошибки от сервера (например, занятый ник)
        if (json.contains("error") && json["error"].toBool()) {
            QString errMsg = json["message"].toString();
            isLobby = false;
            disconnectAll();
            emit lobbyStatusChanged(errMsg);
            return;
        }

        int serverGameType = json["gameType"].toInt();
        if (serverGameType != selectedClientGameType) {
            static const QString gameNames[] = { getLocalizedText("Покера", "Poker"), getLocalizedText("Дурака", "Durak"), getLocalizedText("Козла", "Kozel"), getLocalizedText("Уно", "UNO") };
            QString myGame = gameNames[std::clamp(selectedClientGameType, 0, 3)];
            QString serverGame = gameNames[std::clamp(serverGameType, 0, 3)];

            QString errorMsg = QString(getLocalizedText("Ошибка: Вы выбрали %1, а это лобби %2!", "Error: You selected %1, but this is a %2 lobby!")).arg(myGame, serverGame);

            isLobby = false;
            disconnectAll();
            emit lobbyStatusChanged(errorMsg);
            return;
        }

        isLobby = json["isLobby"].toBool();
        if (isLobby) {
            int count = json["playerCount"].toInt();
            static const QString gameNames[] = { getLocalizedText("ПОКЕРА", "POKER"), getLocalizedText("ДУРАКА", "DURAK"), getLocalizedText("КОЗЛА", "KOZEL"), getLocalizedText("УНО", "UNO") };
            emit lobbyStatusChanged(QString(getLocalizedText("ЛОББИ (%1): %2/4 игроков. Ожидание старта...", "LOBBY (%1): %2/4 players. Waiting to start...")).arg(gameNames[serverGameType]).arg(count));
            continue;
        }

        if (json.contains("yourId")) {
            myIdx = json["yourId"].toInt();
        }

        emit signalClientGameStarted(serverGameType, json);
        emit signalNetworkDataReceived(0, json);
    }
}
