#pragma once

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "NetworkManager.h"

/**
 * @brief Диалоговое окно автоматического поиска и выбора игровых комнат в локальной сети (LAN).
 */
class LanLobbiesDialog : public QDialog {
    Q_OBJECT
public:
    explicit LanLobbiesDialog(NetworkManager* netMgr, QWidget* parent = nullptr);
    ~LanLobbiesDialog() override;

    /**
     * @brief Возвращает выбранное пользователем лобби.
     */
    DiscoveredLobby getSelectedLobby() const { return m_selectedLobby; }

private slots:
    /**
     * @brief Обновление визуального списка доступных серверов при получении UDP-пакетов.
     */
    void updateLobbiesList(const QVector<DiscoveredLobby>& lobbies);

    /**
     * @brief Обработка подтверждения подключения к выбранной комнате.
     */
    void onConnectClicked();

private:
    NetworkManager*        m_netManager = nullptr;
    QListWidget*           m_listWidget = nullptr;
    QPushButton*           m_btnConnect = nullptr;
    QPushButton*           m_btnCancel  = nullptr;
    QLabel*                m_lblStatus  = nullptr;
    QVector<DiscoveredLobby> m_lobbies;
    DiscoveredLobby        m_selectedLobby;
};
