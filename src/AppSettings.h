#pragma once

#include <QString>
#include <QtGlobal>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QStandardPaths>
#include <QDir>

/**
 * @brief Возвращает строку на русском или английском языке в зависимости от локали
 */
inline QString getLocalizedText(const QString& ru, const QString& en) {
    return (QLocale::system().language() == QLocale::Russian) ? ru : en;
}

/**
 * @brief Возвращает путь к файлу настроек в кроссплатформенной директории данных
 */
inline QString getSettingsFilePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/settings.json";
}

/**
 * @brief Перечисление доступных цветовых схем сукна игрового стола.
 */
enum class TableColor {
    ClassicGreen = 0,
    BurgundyRed  = 1,
    DarkBlue     = 2,
    PokerBlack   = 3
};

/**
 * @brief Перечисление стилей рубашек игральных карт.
 */
enum class CardShirtStyle {
    ClassicBlue = 0,
    RedVelvet   = 1,
    GoldRoyal   = 2,
    DarkPattern = 3
};

/**
 * @brief Перечисление доступных аватаров пользователя.
 */
enum class AvatarIcon {
    Crown     = 0,
    Skull     = 1,
    SuitSpade = 2,
    Joker     = 3
};

enum class UnoDrawMode {
    DrawOne        = 0, // Взять 1 карту и пас/ход
    DrawUntilMatch = 1  // Тянуть из колоды до подходящей карты
};

/**
 * @brief Глобальный потокобезопасный синглтон настроек приложения.
 * Обеспечивает сохранение и загрузку параметров в файл settings.json.
 */
class AppSettings {
public:
    /**
     * @brief Получение единственного экземпляра синглтона.
     */
    static AppSettings& instance() {
        static AppSettings inst;
        return inst;
    }

    TableColor tableColor       = TableColor::ClassicGreen;
    CardShirtStyle cardShirt    = CardShirtStyle::ClassicBlue;
    AvatarIcon avatar           = AvatarIcon::Crown;
    UnoDrawMode unoDrawMode     = UnoDrawMode::DrawUntilMatch;
    bool unoStacking            = true; // Перевод и накопление +2 / +4
    bool fullScreen             = false;
    QString nickname            = getLocalizedText("Игрок", "Player");
    bool autoNextHand           = true;
    bool showPokerHandHint      = true;
    quint16 serverPort          = 12345;

    /**
     * @brief Сохранение настроек в формате JSON.
     */
    void save() {
        QJsonObject json;
        json["nickname"]          = nickname;
        json["avatar"]            = static_cast<int>(avatar);
        json["tableColor"]        = static_cast<int>(tableColor);
        json["cardShirt"]         = static_cast<int>(cardShirt);
        json["unoDrawMode"]       = static_cast<int>(unoDrawMode);
        json["unoStacking"]       = unoStacking;
        json["fullScreen"]        = fullScreen;
        json["autoNextHand"]      = autoNextHand;
        json["showPokerHandHint"] = showPokerHandHint;
        json["serverPort"]        = serverPort;

        QFile file(getSettingsFilePath());
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
            file.close();
        }
    }

    /**
     * @brief Загрузка настроек из файла JSON.
     */
    void load() {
        QFile file(getSettingsFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();

            if (doc.isObject()) {
                QJsonObject json = doc.object();
                if (json.contains("nickname"))          nickname          = json["nickname"].toString();
                if (json.contains("avatar"))            avatar            = static_cast<AvatarIcon>(json["avatar"].toInt());
                if (json.contains("tableColor"))        tableColor        = static_cast<TableColor>(json["tableColor"].toInt());
                if (json.contains("cardShirt"))         cardShirt         = static_cast<CardShirtStyle>(json["cardShirt"].toInt());
                if (json.contains("unoDrawMode"))       unoDrawMode       = static_cast<UnoDrawMode>(json["unoDrawMode"].toInt());
                if (json.contains("unoStacking"))       unoStacking       = json["unoStacking"].toBool(true);
                if (json.contains("fullScreen"))        fullScreen        = json["fullScreen"].toBool();
                if (json.contains("autoNextHand"))      autoNextHand      = json["autoNextHand"].toBool();
                if (json.contains("showPokerHandHint")) showPokerHandHint = json["showPokerHandHint"].toBool();
                if (json.contains("serverPort"))        serverPort        = static_cast<quint16>(json["serverPort"].toInt());
            }
        }
    }

private:
    AppSettings() {
        load();
    }
    ~AppSettings() = default;
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;
};
