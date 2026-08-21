#include <QApplication>
#include <QIcon>
#include <QFontDatabase>

#include "MainWindow.h"
#include "Audio.h"

#if defined(Q_OS_IOS)
#include <objc/runtime.h>
#include <objc/message.h>

/**
 * @brief Настройка нативного поведения iOS: скрытие Home Indicator и запрет автоблокировки экрана.
 */
inline void setupIosNativeBehavior() {
    // 1. Запрет отключения подсветки дисплея
    Class appClass = objc_getClass("UIApplication");
    SEL sharedAppSel = sel_registerName("sharedApplication");
    id app = reinterpret_cast<id(*)(Class, SEL)>(objc_msgSend)(appClass, sharedAppSel);
    if (app) {
        SEL setIdleTimerSel = sel_registerName("setIdleTimerDisabled:");
        reinterpret_cast<void(*)(id, SEL, bool)>(objc_msgSend)(app, setIdleTimerSel, true);
    }

    // 2. Автоматическое скрытие нижней системной полосы (Home Indicator)
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

/**
 * @brief Настройка нативного поведения Android: Immersive Sticky Mode, разворачивание под вырез камеры и Wake Lock.
 */
inline void setupAndroidNativeBehavior() {
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
        auto activity = QJniObject(QNativeInterface::QAndroidApplication::context());
        if (!activity.isValid()) return;

        QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (!window.isValid()) return;

        // 1. Удержание экрана включенным во время игры (FLAG_KEEP_SCREEN_ON = 128)
        const int FLAG_KEEP_SCREEN_ON = 128;
        window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);

        // 2. Растягивание окна под вырез камеры (Display Cutout Mode = SHORT_EDGES)
        QJniObject layoutParams = window.callObjectMethod("getAttributes", "()Landroid/view/WindowManager$LayoutParams;");
        if (layoutParams.isValid()) {
            layoutParams.setField<int>("layoutInDisplayCutoutMode", 1);
            window.callMethod<void>("setAttributes", "(Landroid/view/WindowManager$LayoutParams;)V", layoutParams.object<jobject>());
        }

        // 3. Полное скрытие системных панелей навигации (Immersive Sticky Mode)
        QJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
        if (decorView.isValid()) {
            const int SYSTEM_UI_FLAG_LAYOUT_STABLE = 256;
            const int SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = 512;
            const int SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = 1024;
            const int SYSTEM_UI_FLAG_HIDE_NAVIGATION = 2;
            const int SYSTEM_UI_FLAG_FULLSCREEN = 4;
            const int SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 4096;

            const int flags = SYSTEM_UI_FLAG_LAYOUT_STABLE |
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
 * @brief Главная точка входа приложения.
 */
int main(int argc, char *argv[]) {
    QApplication application(argc, argv);

    // Установка логотипа приложения
    application.setWindowIcon(QIcon(":/icon.png"));

    // Инициализация и регистрация встроенного шрифта Noto Sans
    const int fontId = QFontDatabase::addApplicationFont(":/NotoSans.ttf");
    if (fontId != -1) {
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            QFont appFont(families.first());
            application.setFont(appFont);
        }
    }

    #if defined(Q_OS_ANDROID)
    // Обработка сворачивания и разворачивания приложения для фоновой музыки
    QObject::connect(&application, &QGuiApplication::applicationStateChanged, [](Qt::ApplicationState state) {
        if (state == Qt::ApplicationSuspended || state == Qt::ApplicationHidden) {
            AudioManager::instance().pauseMusic();
        } else if (state == Qt::ApplicationActive) {
            setupAndroidNativeBehavior();
            AudioManager::instance().resumeMusic();
        }
    });
    #endif

    // Инициализация главного окна
    MainWindow window;

    #if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    window.setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
    window.showFullScreen();

    #if defined(Q_OS_IOS)
    setupIosNativeBehavior();
    #elif defined(Q_OS_ANDROID)
    setupAndroidNativeBehavior();
    #endif
    #else
    if (AppSettings::instance().getFullScreen()) {
        window.showFullScreen();
    } else {
        window.show();
    }
    #endif

    // Запуск фонового джазового саундтрека
    AudioManager::instance().startMusic();

    return application.exec();
}
