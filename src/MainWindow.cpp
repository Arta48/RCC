#include "MainWindow.h"
#include "AppSettings.h"
#include "Audio.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/RulesDialog.h"

#include <QVBoxLayout>

#if defined(Q_OS_ANDROID)
#include <QOpenGLWidget>
#endif

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    // Установка заголовка окна в зависимости от локали
    setWindowTitle(getLocalizedText("Royal Card Club Collection", "Royal Card Club Collection"));

    #if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    // Базовые габариты для десктопных платформ (Windows, Linux, macOS)
    resize(1280, 720);
    setMinimumSize(640, 360);

    // Горячая клавиша F11 для переключения полноэкранного режима
    auto* f11Shortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(f11Shortcut, &QShortcut::activated, this, [this]() {
        if (isFullScreen()) {
            showNormal();
            AppSettings::instance().setFullScreen(false);
        } else {
            showFullScreen();
            AppSettings::instance().setFullScreen(true);
        }
        AppSettings::instance().save();
    });
    #endif

    // Инициализация сетевого менеджера
    netManager = new NetworkManager(this);

    // Инициализация стека виджетов и игровых экранов
    stackedWidget = new QStackedWidget(this);
    menuWidget    = new MainMenuWidget(this);
    pokerWidget   = new PokerWidget(netManager, this);
    durakWidget   = new DurakWidget(netManager, this);
    kozelWidget   = new KozelWidget(netManager, this);
    unoWidget     = new UnoWidget(netManager, this);

    // Добавляем экраны в строго определенном порядке индексов
    stackedWidget->addWidget(menuWidget);  // Индекс 0
    stackedWidget->addWidget(pokerWidget); // Индекс 1
    stackedWidget->addWidget(durakWidget); // Индекс 2
    stackedWidget->addWidget(kozelWidget); // Индекс 3
    stackedWidget->addWidget(unoWidget);   // Индекс 4

    #if defined(Q_OS_ANDROID)
    // Для Android оборачиваем стек в QOpenGLWidget для предотвращения артефактов растеризации (QTBUG-127495)
    auto* glContainer = new QOpenGLWidget(this);
    auto* glLayout = new QVBoxLayout(glContainer);
    glLayout->setContentsMargins(0, 0, 0, 0);
    glLayout->addWidget(stackedWidget);
    setCentralWidget(glContainer);
    #else
    setCentralWidget(stackedWidget);
    #endif

    // =========================================================================
    // СИГНАЛЫ СТАРТА СЕТЕВЫХ ИГР (НА СТОРОНЕ ХОСТА)
    // =========================================================================
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

    // =========================================================================
    // СИГНАЛЫ СТАРТА ИГРЫ (НА СТОРОНЕ КЛИЕНТА)
    // =========================================================================
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

    // =========================================================================
    // КНОПКИ ГЛАВНОГО МЕНЮ: ОДИНОЧНАЯ ИГРА С БОТАМИ
    // =========================================================================
    connect(menuWidget->btnStartBotGame, &QPushButton::clicked, this, [this]() {
        const int gameType = menuWidget->comboGameType->currentData().toInt();
        const int botCount = menuWidget->comboBots->currentData().toInt();

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

    // =========================================================================
    // КНОПКИ ГЛАВНОГО МЕНЮ: СОЗДАНИЕ СЕРВЕРА (ХОСТ)
    // =========================================================================
    connect(menuWidget->btnHostServer, &QPushButton::clicked, this, [this]() {
        // 1. Остановка фоновых таймеров ИИ
        pokerWidget->aiTimer->stop();
        durakWidget->aiTimer->stop();
        kozelWidget->aiTimer->stop();
        unoWidget->aiTimer->stop();

        // 2. Сброс состояния всех движков
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

        // 3. Запуск сервера с выбранным типом игры
        const int gameType = menuWidget->comboGameType->currentData().toInt();
        netManager->startHostServer(gameType);

        pokerWidget->updateUI();
        stackedWidget->setCurrentIndex(1);
    });

    // =========================================================================
    // КНОПКИ ГЛАВНОГО МЕНЮ: ПОДКЛЮЧЕНИЕ ПО IP (КЛИЕНТ)
    // =========================================================================
    connect(menuWidget->btnConnectIP, &QPushButton::clicked, this, [this]() {
        pokerWidget->aiTimer->stop();
        durakWidget->aiTimer->stop();
        kozelWidget->aiTimer->stop();
        unoWidget->aiTimer->stop();

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

        const int gameType = menuWidget->comboGameType->currentData().toInt();
        netManager->connectToHost(menuWidget->ipInput->text().trimmed(), gameType);

        pokerWidget->updateUI();
        stackedWidget->setCurrentIndex(1);
    });

    // =========================================================================
    // СЕТЕВАЯ СИНХРОНИЗАЦИЯ И РЕКОННЕКТ
    // =========================================================================
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
        const QString discMsg = getLocalizedText("Связь с сервером потеряна! Хост отключился.", "Connection lost! Host disconnected.");

        pokerWidget->engine.gameOver = true;
        pokerWidget->engine.statusMessage = discMsg;

        durakWidget->engine.gameOver = true;
        durakWidget->engine.statusMessage = discMsg;

        kozelWidget->engine.gameOver = true;
        kozelWidget->engine.statusMessage = discMsg;

        unoWidget->engine.gameOver = true;
        unoWidget->engine.statusMessage = discMsg;

        if (stackedWidget->currentIndex() == 1) pokerWidget->updateUI();
        else if (stackedWidget->currentIndex() == 2) durakWidget->updateUI();
        else if (stackedWidget->currentIndex() == 3) kozelWidget->updateUI();
        else if (stackedWidget->currentIndex() == 4) unoWidget->updateUI();
    });

        connect(netManager, &NetworkManager::signalNetworkDataReceived, this, [this](int senderId, const QJsonObject& json) {
            if (!netManager->isHost && stackedWidget->currentIndex() == 0) return;

            if (netManager->isHost) {
                const QString act = json["act"].toString();

                // Обработка обновления профиля игрока (имя/аватар) прямо во время матча
                if (act == "UPDATE_PROFILE") {
                    QString newName = json["name"].toString().trimmed();
                    const int newAvatar = json["avatar"].toInt(0);
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
                const int gType = json["gameType"].toInt();
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

        // =========================================================================
        // CALLBACK: ВОЗВРАТ В ЛОББИ ПОСЛЕ ЗАВЕРШЕНИЯ МАТЧА
        // =========================================================================
        auto returnToLobby = [this]() {
            netManager->isLobby = true;
            pokerWidget->engine.gameOver = false;
            pokerWidget->engine.players.resize(1);
            pokerWidget->engine.players[0].id = 0;
            pokerWidget->engine.players[0].name = AppSettings::instance().getNickname();
            pokerWidget->engine.players[0].avatar = static_cast<int>(AppSettings::instance().getAvatar());
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

            static const QString gameNames[] = {
                getLocalizedText("ПОКЕРА", "POKER"),
                getLocalizedText("ДУРАКА", "DURAK"),
                getLocalizedText("КОЗЛА", "KOZEL"),
                getLocalizedText("УНО", "UNO")
            };
            const QString statusMsg = QString(getLocalizedText("ЛОББИ (%1): 1/%2 игроков. Ожидание...", "LOBBY (%1): 1/%2 players. Waiting..."))
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

        // =========================================================================
        // CALLBACK: ПОЛНЫЙ ВЫХОД В ГЛАВНОЕ МЕНЮ
        // =========================================================================
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

        // =========================================================================
        // CALLBACK: МГНОВЕННОЕ ПРИМЕНЕНИЕ НАСТРОЕК В РЕЖИМЕ РЕАЛЬНОГО ВРЕМЕНИ
        // =========================================================================
        auto applySettingsChanges = [this]() {
            auto updateLocalPlayer = [](auto& engine) {
                if (engine.myIdx < engine.players.size()) {
                    engine.players[engine.myIdx].name   = AppSettings::instance().getNickname();
                    engine.players[engine.myIdx].avatar = static_cast<int>(AppSettings::instance().getAvatar());
                }
            };
            updateLocalPlayer(pokerWidget->engine);
            updateLocalPlayer(durakWidget->engine);
            updateLocalPlayer(kozelWidget->engine);
            updateLocalPlayer(unoWidget->engine);

            unoWidget->engine.drawMode        = AppSettings::instance().getUnoDrawMode();
            unoWidget->engine.stackingEnabled = AppSettings::instance().getUnoStacking();

            if (netManager && netManager->isNetworkGame) {
                if (netManager->isHost) {
                    if (!netManager->lobbyClients.isEmpty()) {
                        netManager->lobbyClients[0].name   = AppSettings::instance().getNickname();
                        netManager->lobbyClients[0].avatar = static_cast<int>(AppSettings::instance().getAvatar());
                    }
                    if (netManager->gameType == 0)      pokerWidget->broadcastNetState();
                    else if (netManager->gameType == 1) durakWidget->broadcastNetState();
                    else if (netManager->gameType == 2) kozelWidget->broadcastNetState();
                    else if (netManager->gameType == 3) unoWidget->broadcastNetState();
                } else {
                    QJsonObject json;
                    json["act"]    = "UPDATE_PROFILE";
                    json["name"]   = AppSettings::instance().getNickname();
                    json["avatar"] = static_cast<int>(AppSettings::instance().getAvatar());
                    netManager->sendJsonToServer(json);
                }
            }

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

        pokerWidget->onOpenSettingsCallback = openInGameSettings;
        durakWidget->onOpenSettingsCallback = openInGameSettings;
        kozelWidget->onOpenSettingsCallback = openInGameSettings;
        unoWidget->onOpenSettingsCallback   = openInGameSettings;

        connect(menuWidget->btnRules, &QPushButton::clicked, this, [this]() {
            AudioManager::instance().playSound(SoundEffect::ButtonClick);
            const int curGame = menuWidget->comboGameType->currentData().toInt();
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
