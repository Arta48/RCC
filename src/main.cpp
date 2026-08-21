#include <QApplication>
#include <QIcon>
#include <QFontDatabase>
#include "MainWindow.h"
#include "Audio.h"

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
        class_replaceMethod(vcClass, sel_registerName("prefersHomeIndicatorAutoHidden"), reinterpret_cast<IMP>(+autoHideImp), "c@:");

        auto deferGesturesImp = [](id, SEL) -> unsigned long { return 15; /* UIRectEdgeAll */ };
        class_replaceMethod(vcClass, sel_registerName("preferredScreenEdgesDeferringSystemGestures"), reinterpret_cast<IMP>(+deferGesturesImp), "Q@:");
    }
}
#elif defined(Q_OS_ANDROID)
#include <QJniObject>

inline void setupAndroidNativeBehavior() {
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
        auto activity = QJniObject(QNativeInterface::QAndroidApplication::context());
        if (!activity.isValid()) return;

        QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (!window.isValid()) return;

        // 1. Запрещаем гасить экран
        const int FLAG_KEEP_SCREEN_ON = 128;
        window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);

        // 2. Растягиваем окно под вырез камеры (Samsung Infinity-U и др.)
        QJniObject layoutParams = window.callObjectMethod("getAttributes", "()Landroid/view/WindowManager$LayoutParams;");
        if (layoutParams.isValid()) {
            layoutParams.setField<int>("layoutInDisplayCutoutMode", 1); // SHORT_EDGES
            window.callMethod<void>("setAttributes", "(Landroid/view/WindowManager$LayoutParams;)V", layoutParams.object<jobject>());
        }

        // 3. Скрываем навигационный бар (Immersive Sticky Mode)
        QJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
        if (decorView.isValid()) {
            const int SYSTEM_UI_FLAG_LAYOUT_STABLE = 256;
            const int SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = 512;
            const int SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = 1024;
            const int SYSTEM_UI_FLAG_HIDE_NAVIGATION = 2;
            const int SYSTEM_UI_FLAG_FULLSCREEN = 4;
            const int SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 4096;

            int flags = SYSTEM_UI_FLAG_LAYOUT_STABLE |
            SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
            SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
            SYSTEM_UI_FLAG_HIDE_NAVIGATION |
            SYSTEM_UI_FLAG_FULLSCREEN |
            SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

            decorView.callMethod<void>("setSystemUiVisibility", "(I)V", flags);
        }
    });
}
#endif

/**
 * @brief Точка входа в программу.
 */
int main(int argc, char *argv[]) {
    QApplication application(argc, argv);

    // Установка иконки приложения из скомпилированных ресурсов Qt
    application.setWindowIcon(QIcon(":/icon.png"));

    // Инициализация встроенного шрифта Noto Sans
    int fontId = QFontDatabase::addApplicationFont(":/NotoSans.ttf");
    if (fontId != -1) {
        QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            QFont appFont(families.first());
            application.setFont(appFont);
        }
    }

#if defined(Q_OS_ANDROID)
    // Пауза при скрытии / сворачивании (Музыка)
    QObject::connect(&application, &QGuiApplication::applicationStateChanged, [](Qt::ApplicationState state) {
        if (state == Qt::ApplicationSuspended || state == Qt::ApplicationHidden) {
            AudioManager::instance().pauseMusic();
        } else if (state == Qt::ApplicationActive) {
            setupAndroidNativeBehavior();
            AudioManager::instance().resumeMusic();
        }
    });
#endif

    // Инициализация и отображение главного окна
    MainWindow window;
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    // Отключаем системные отступы Safe Area
    window.setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
    window.showFullScreen();

    #if defined(Q_OS_IOS)
    setupIosNativeBehavior();
    #elif defined(Q_OS_ANDROID)
    setupAndroidNativeBehavior();
    #endif
#else
    window.show();
#endif
    AudioManager::instance().startMusic();

    return application.exec();
}
