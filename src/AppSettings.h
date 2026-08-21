#pragma once

#include <QString>
#include <QtGlobal>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QStandardPaths>
#include <QDir>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>

/**
 * @brief Возвращает строку на русском или английском языке в зависимости от системной локали.
 * @param ru Текст на русском языке.
 * @param en Текст на английском языке.
 * @return Локализованная строка.
 */
inline QString getLocalizedText(const QString& ru, const QString& en) {
    return (QLocale::system().language() == QLocale::Russian) ? ru : en;
}

/**
 * @brief Возвращает абсолютный путь к файлу настроек в стандартной кроссплатформенной директории AppData.
 * Создает директорию, если она еще не существует.
 */
inline QString getSettingsFilePath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/settings.json";
}

/**
 * @brief Перечисление доступных цветовых тем сукна игрового стола.
 */
enum class TableColor {
    ClassicGreen = 0, ///< Классическое зеленое сукно
    BurgundyRed  = 1, ///< Бордовый бархат
    DarkBlue     = 2, ///< Темно-синий атлас
    PokerBlack   = 3  ///< Премиальный черный покерный стол
};

/**
 * @brief Перечисление стилей рубашек игральных карт.
 */
enum class CardShirtStyle {
    ClassicBlue = 0, ///< Синий геометрический узор
    RedVelvet   = 1, ///< Красный бархат
    GoldRoyal   = 2, ///< Золотой королевский орнамент
    DarkPattern = 3  ///< Карбоновый темный паттерн
};

/**
 * @brief Перечисление доступных аватаров пользователя.
 */
enum class AvatarIcon {
    Crown     = 0, ///< 👑 Корона
    Skull     = 1, ///< 💀 Череп
    SuitSpade = 2, ///< ♠ Пики
    Joker     = 3  ///< 🃏 Джокер
};

/**
 * @brief Режим взятия карт из колоды в игре Уно.
 */
enum class UnoDrawMode {
    DrawOne        = 0, ///< Взять 1 карту и сделать ход/пас
    DrawUntilMatch = 1  ///< Тянуть из колоды до выпадения подходящей карты
};

/**
 * @brief Глобальный потокобезопасный синглтон настроек приложения (C++17 / Qt6).
 * Обеспечивает синхронизированный доступ к параметрам приложения из UI, сетевого и фонового потоков
 * с помощью QReadWriteLock.
 */
class AppSettings {
public:
    /**
     * @brief Получение единственного глобального экземпляра синглтона.
     */
    static AppSettings& instance() {
        static AppSettings inst;
        return inst;
    }

    // =========================================================================
    // Потокобезопасные Геттеры (Read Lock)
    // =========================================================================

    TableColor getTableColor() const {
        QReadLocker locker(&m_lock);
        return m_tableColor;
    }

    CardShirtStyle getCardShirt() const {
        QReadLocker locker(&m_lock);
        return m_cardShirt;
    }

    AvatarIcon getAvatar() const {
        QReadLocker locker(&m_lock);
        return m_avatar;
    }

    UnoDrawMode getUnoDrawMode() const {
        QReadLocker locker(&m_lock);
        return m_unoDrawMode;
    }

    bool getUnoStacking() const {
        QReadLocker locker(&m_lock);
        return m_unoStacking;
    }

    bool getFullScreen() const {
        QReadLocker locker(&m_lock);
        return m_fullScreen;
    }

    QString getNickname() const {
        QReadLocker locker(&m_lock);
        return m_nickname;
    }

    bool getAutoNextHand() const {
        QReadLocker locker(&m_lock);
        return m_autoNextHand;
    }

    bool getShowPokerHandHint() const {
        QReadLocker locker(&m_lock);
        return m_showPokerHandHint;
    }

    quint16 getServerPort() const {
        QReadLocker locker(&m_lock);
        return m_serverPort;
    }

    // =========================================================================
    // Потокобезопасные Сеттеры (Write Lock)
    // =========================================================================

    void setTableColor(TableColor value) {
        QWriteLocker locker(&m_lock);
        m_tableColor = value;
    }

    void setCardShirt(CardShirtStyle value) {
        QWriteLocker locker(&m_lock);
        m_cardShirt = value;
    }

    void setAvatar(AvatarIcon value) {
        QWriteLocker locker(&m_lock);
        m_avatar = value;
    }

    void setUnoDrawMode(UnoDrawMode value) {
        QWriteLocker locker(&m_lock);
        m_unoDrawMode = value;
    }

    void setUnoStacking(bool value) {
        QWriteLocker locker(&m_lock);
        m_unoStacking = value;
    }

    void setFullScreen(bool value) {
        QWriteLocker locker(&m_lock);
        m_fullScreen = value;
    }

    void setNickname(const QString& value) {
        QWriteLocker locker(&m_lock);
        m_nickname = value;
    }

    void setAutoNextHand(bool value) {
        QWriteLocker locker(&m_lock);
        m_autoNextHand = value;
    }

    void setShowPokerHandHint(bool value) {
        QWriteLocker locker(&m_lock);
        m_showPokerHandHint = value;
    }

    void setServerPort(quint16 value) {
        QWriteLocker locker(&m_lock);
        m_serverPort = value;
    }

    // =========================================================================
    // Сериализация в файл (JSON)
    // =========================================================================

    /**
     * @brief Сохранение всех текущих настроек в файл settings.json.
     */
    void save() {
        QJsonObject json;
        {
            QReadLocker locker(&m_lock);
            json["nickname"]          = m_nickname;
            json["avatar"]            = static_cast<int>(m_avatar);
            json["tableColor"]        = static_cast<int>(m_tableColor);
            json["cardShirt"]         = static_cast<int>(m_cardShirt);
            json["unoDrawMode"]       = static_cast<int>(m_unoDrawMode);
            json["unoStacking"]       = m_unoStacking;
            json["fullScreen"]        = m_fullScreen;
            json["autoNextHand"]      = m_autoNextHand;
            json["showPokerHandHint"] = m_showPokerHandHint;
            json["serverPort"]        = m_serverPort;
        }

        QFile file(getSettingsFilePath());
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
            file.close();
        }
    }

    /**
     * @brief Загрузка сохраненных настроек из файла JSON.
     */
    void load() {
        QFile file(getSettingsFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();

            if (doc.isObject()) {
                const QJsonObject json = doc.object();
                QWriteLocker locker(&m_lock);

                if (json.contains("nickname"))          m_nickname          = json["nickname"].toString();
                if (json.contains("avatar"))            m_avatar            = static_cast<AvatarIcon>(json["avatar"].toInt());
                if (json.contains("tableColor"))        m_tableColor        = static_cast<TableColor>(json["tableColor"].toInt());
                if (json.contains("cardShirt"))         m_cardShirt         = static_cast<CardShirtStyle>(json["cardShirt"].toInt());
                if (json.contains("unoDrawMode"))       m_unoDrawMode       = static_cast<UnoDrawMode>(json["unoDrawMode"].toInt());
                if (json.contains("unoStacking"))       m_unoStacking       = json["unoStacking"].toBool(true);
                if (json.contains("fullScreen"))        m_fullScreen        = json["fullScreen"].toBool();
                if (json.contains("autoNextHand"))      m_autoNextHand      = json["autoNextHand"].toBool();
                if (json.contains("showPokerHandHint")) m_showPokerHandHint = json["showPokerHandHint"].toBool();
                if (json.contains("serverPort"))        m_serverPort        = static_cast<quint16>(json["serverPort"].toInt(12345));
            }
        }
    }

private:
    AppSettings() {
        m_nickname = getLocalizedText("Игрок", "Player");
        load();
    }
    ~AppSettings() = default;
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;

    mutable QReadWriteLock m_lock;

    TableColor     m_tableColor       = TableColor::ClassicGreen;
    CardShirtStyle m_cardShirt        = CardShirtStyle::ClassicBlue;
    AvatarIcon     m_avatar           = AvatarIcon::Crown;
    UnoDrawMode    m_unoDrawMode      = UnoDrawMode::DrawUntilMatch;
    bool           m_unoStacking      = true;
    bool           m_fullScreen       = false;
    QString        m_nickname;
    bool           m_autoNextHand     = true;
    bool           m_showPokerHandHint = true;
    quint16        m_serverPort       = 12345;
};
