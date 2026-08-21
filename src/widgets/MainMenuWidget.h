#pragma once

#include <QWidget>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QLineEdit>
#include <QPaintEvent>
#include <QResizeEvent>

#include "TouchComboBox.h"

/**
 * @brief Главное меню выбора карточной игры, параметров ботов, хостинга и подключения по LAN/IP.
 */
class MainMenuWidget : public QWidget {
    Q_OBJECT
public:
    QLabel*        lblTitle;
    QLabel*        lblSub;
    QLabel*        lblSelectHeader;
    QLabel*        lblSingleHeader;
    QLabel*        lblOpponents;
    QLabel*        lblMultiHeader;
    QFrame*        selectFrame;
    QFrame*        botFrame;
    QFrame*        netFrame;
    TouchComboBox* comboGameType;
    TouchComboBox* comboBots;
    QPushButton*   btnStartBotGame;
    QPushButton*   btnHostServer;
    QPushButton*   btnConnectIP;
    QPushButton*   btnSettings;
    QPushButton*   btnRules;
    QLineEdit*     ipInput;

    explicit MainMenuWidget(QWidget* parent = nullptr);
    ~MainMenuWidget() override = default;

protected:
    void paintEvent(QPaintEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
};
