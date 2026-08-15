#include "Audio.h"

AudioManager& AudioManager::instance() {
    static AudioManager inst;
    return inst;
}

AudioManager::AudioManager(QObject* parent) : QObject(parent) {
    m_bgPlayer = new QMediaPlayer(this);
    m_bgAudioOutput = new QAudioOutput(this);

    m_bgPlayer->setAudioOutput(m_bgAudioOutput);
    m_bgPlayer->setSource(QUrl("qrc:/audio/bg_jazz.mp3"));
    m_bgPlayer->setLoops(QMediaPlayer::Infinite);
    m_bgAudioOutput->setVolume(m_musicVolume);

    initEffects();
}

void AudioManager::initEffects() {
    struct EffectMapping {
        SoundEffect type;
        QString path;
    };

    const EffectMapping mappings[] = {
        { SoundEffect::ButtonClick, "qrc:/audio/button_click.wav" },
        { SoundEffect::CardShuffle, "qrc:/audio/card_shuffle.wav" },
        { SoundEffect::CardPlace,   "qrc:/audio/card_place.wav" },
        { SoundEffect::ChipBet,     "qrc:/audio/chip_bet.wav" },
        { SoundEffect::Check,       "qrc:/audio/check_tap.wav" },
        { SoundEffect::Fold,        "qrc:/audio/card_fold.wav" },
        { SoundEffect::Win,         "qrc:/audio/win_chips.wav" },
        { SoundEffect::Lose,        "qrc:/audio/lose.wav" },
        { SoundEffect::Bito,        "qrc:/audio/bito.wav" }
    };

    for (const auto& item : mappings) {
        auto* player = new QMediaPlayer(this);
        auto* audioOutput = new QAudioOutput(this);

        player->setAudioOutput(audioOutput);
        player->setSource(QUrl(item.path));
        audioOutput->setVolume(m_sfxVolume);

        m_effects.insert(item.type, { player, audioOutput });
    }
}

void AudioManager::playSound(SoundEffect effect) {
    if (m_sfxMuted) return;

    if (m_effects.contains(effect)) {
        auto& sfx = m_effects[effect];
        sfx.player->setPosition(0);
        sfx.player->play();
    }
}

void AudioManager::startMusic() {
    if (!m_musicMuted && m_bgPlayer->playbackState() != QMediaPlayer::PlayingState) {
        m_bgPlayer->play();
    }
}

void AudioManager::stopMusic() {
    m_bgPlayer->stop();
}

void AudioManager::setMusicVolume(float volume) {
    m_musicVolume = qBound(0.0f, volume, 1.0f);
    if (!m_musicMuted) {
        m_bgAudioOutput->setVolume(m_musicVolume);
    }
}

void AudioManager::setSfxVolume(float volume) {
    m_sfxVolume = qBound(0.0f, volume, 1.0f);
    for (auto& sfx : m_effects) {
        sfx.audioOutput->setVolume(m_sfxVolume);
    }
}

void AudioManager::toggleMuteMusic() {
    m_musicMuted = !m_musicMuted;
    if (m_musicMuted) {
        m_bgAudioOutput->setVolume(0.0f);
    } else {
        m_bgAudioOutput->setVolume(m_musicVolume);
        startMusic();
    }
}

void AudioManager::toggleMuteSfx() {
    m_sfxMuted = !m_sfxMuted;
}
