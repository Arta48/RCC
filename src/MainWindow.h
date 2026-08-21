#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QShortcut>

#include "NetworkManager.h"
#include "widgets/MainMenuWidget.h"
#include "widgets/PokerWidget.h"
#include "widgets/DurakWidget.h"
#include "widgets/KozelWidget.h"
#include "widgets/UnoWidget.h"

/**
 * @brief Главное окно приложения, координирующее переключение экранов,
 * жизненный цикл игровых столов и сетевую маршрутизацию сообщений.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    QStackedWidget* stackedWidget;  // Контейнер переключения экранов (0: Меню, 1: Покер, 2: Дурак, 3: Козёл, 4: Уно)
    MainMenuWidget* menuWidget;     // Экран главного меню
    PokerWidget*    pokerWidget;    // Экран игрового стола Покера
    DurakWidget*    durakWidget;    // Экран игрового стола Подкидного Дурака
    KozelWidget*    kozelWidget;    // Экран игрового стола Козла
    UnoWidget*      unoWidget;      // Экран игрового стола Уно
    NetworkManager* netManager;     // Сетевой менеджер TCP (Хост / Клиент)

    /**
     * @brief Конструктор главного окна.
     * @param parent Родительский виджет.
     */
    explicit MainWindow(QWidget* parent = nullptr);

    /**
     * @brief Деструктор главного окна с корректным освобождением сетевых ресурсов.
     */
    ~MainWindow() override;
};
