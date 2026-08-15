#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMap>
#include <QUrl>

/**
 * @brief Перечисление звуковых эффектов игры.
 */
enum class SoundEffect {
    ButtonClick,
    CardShuffle,
    CardPlace,
    ChipBet,
    Check,
    Fold,
    Win,
    Lose,
    Bito
};

/**
 * @brief Структура хранения связки проигрывателя и аудиовыхода для SFX.
 */
struct SfxItem {
    QMediaPlayer* player = nullptr;
    QAudioOutput* audioOutput = nullptr;
};

/**
 * @brief Менеджер аудиосистемы приложения (Синглтон).
 * Управляет фоновой музыкой и звуковыми эффектами.
 */
class AudioManager : public QObject {
    Q_OBJECT
public:
    static AudioManager& instance();

    void playSound(SoundEffect effect);
    void startMusic();
    void stopMusic();
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

    QMediaPlayer* m_bgPlayer = nullptr;
    QAudioOutput* m_bgAudioOutput = nullptr;
    QMap<SoundEffect, SfxItem> m_effects;

    float m_musicVolume = 0.05f;
    float m_sfxVolume   = 0.25f;
    bool  m_musicMuted  = false;
    bool  m_sfxMuted    = false;
};
