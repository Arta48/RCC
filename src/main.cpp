#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

#if defined(Q_OS_IOS)
#include <objc/runtime.h>
#include <objc/message.h>

// Управление экраном и скрытие нижней полосы iOS
inline void setupIosNativeBehavior() {
    // 1. Запрещаем гасить экран (setIdleTimerDisabled:YES)
    Class appClass = objc_getClass("UIApplication");
    SEL sharedAppSel = sel_registerName("sharedApplication");
    id app = reinterpret_cast<id(*)(Class, SEL)>(objc_msgSend)(appClass, sharedAppSel);
    if (app) {
        SEL setIdleTimerSel = sel_registerName("setIdleTimerDisabled:");
        reinterpret_cast<void(*)(id, SEL, bool)>(objc_msgSend)(app, setIdleTimerSel, true);
    }

    // 2. Автоматически скрываем белую полосу снизу (Home Indicator)
    Class vcClass = objc_getClass("UIViewController");
    if (vcClass) {
        auto autoHideImp = [](id, SEL) -> bool { return true; };
        class_replaceMethod(vcClass, sel_registerName("prefersHomeIndicatorAutoHidden"),
                            reinterpret_cast<IMP>(+autoHideImp), "c@:");

        auto deferGesturesImp = [](id, SEL) -> unsigned long { return 15; /* UIRectEdgeAll */ };
        class_replaceMethod(vcClass, sel_registerName("preferredScreenEdgesDeferringSystemGestures"),
                            reinterpret_cast<IMP>(+deferGesturesImp), "Q@:");
    }
}
#elif defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QCoreApplication>
inline void setupAndroidNativeBehavior() {
    auto activity = QJniObject(QNativeInterface::QAndroidApplication::context());
    if (activity.isValid()) {
        QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (window.isValid()) {
            const int FLAG_KEEP_SCREEN_ON = 128;
            window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        }
    }
}
#endif

/**
 * @brief Точка входа в программу.
 */
int main(int argc, char *argv[]) {
    QApplication application(argc, argv);

    // Установка иконки приложения из скомпилированных ресурсов Qt
    application.setWindowIcon(QIcon(":/icon.png"));

#if defined(Q_OS_IOS)
    setupIosNativeBehavior();
#elif defined(Q_OS_ANDROID)
    setupAndroidNativeBehavior();
#endif

    // Инициализация и отображение главного окна
    MainWindow window;
#if defined(Q_OS_IOS)
    // Отключаем системные отступы Safe Area — игра займет 100% экрана от края до края
    window.setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
    window.showFullScreen();
#elif defined(Q_OS_ANDROID)
    window.showFullScreen();
#else
    window.show();
#endif

    return application.exec();
}
