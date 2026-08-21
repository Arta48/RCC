#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QMap>
#include <QUrl>

/**
 * @brief Перечисление звуковых эффектов игры.
 */
enum class SoundEffect {
    ButtonClick, ///< Клик по кнопкам интерфейса
    CardShuffle, ///< Перетасовка колоды
    CardPlace,   ///< Выкладывание карты на стол
    ChipBet,     ///< Ставка фишек в банке
    Check,       ///< Стук по столу (Чек / Объявление Уно)
    Fold,        ///< Сброс карт в пас
    Win,         ///< Звук победы
    Lose,        ///< Звук поражения / взятия карт
    Bito         ///< Звук отбоя карт (Бито)
};

/**
 * @brief Контейнер обертки для экземпляра звукового эффекта.
 */
struct SfxItem {
    QSoundEffect* effect = nullptr;
};

/**
 * @brief Глобальный менеджер аудиосистемы приложения (Синглтон).
 * Обеспечивает циклическое воспроизведение фоновой музыки и воспроизведение SFX-эффектов с независимой громкостью.
 */
class AudioManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Получение глобального экземпляра AudioManager.
     */
    static AudioManager& instance();

    void playSound(SoundEffect effect);
    void startMusic();
    void stopMusic();
    void pauseMusic();
    void resumeMusic();
    void setMusicVolume(float volume);
    void setSfxVolume(float volume);
    void toggleMuteMusic();
    void toggleMuteSfx();

    bool isMusicMuted() const { return m_musicMuted; }
    bool isSfxMuted() const { return m_sfxMuted; }

    float getMusicVolume() const { return m_musicVolume; }
    float getSfxVolume() const { return m_sfxVolume; }

private:
    explicit AudioManager(QObject* parent = nullptr);
    ~AudioManager() override = default;
    Q_DISABLE_COPY_MOVE(AudioManager)

    void initEffects();

    QMediaPlayer*            m_bgPlayer      = nullptr;
    QAudioOutput*            m_bgAudioOutput = nullptr;
    QMap<SoundEffect, SfxItem> m_effects;

    float m_musicVolume = 0.05f;
    float m_sfxVolume   = 0.25f;
    bool  m_musicMuted  = false;
    bool  m_sfxMuted    = false;
};
