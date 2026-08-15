#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

/**
 * @brief Точка входа в программу.
 */
int main(int argc, char *argv[]) {
    QApplication application(argc, argv);

    // Установка иконки приложения из скомпилированных ресурсов Qt
    application.setWindowIcon(QIcon(":/icon.png"));

    // Инициализация и отображение главного окна
    MainWindow window;
    window.show();

    return application.exec();
}
