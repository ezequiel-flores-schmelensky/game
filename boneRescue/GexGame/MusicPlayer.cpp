//
// Created by David Burchill on 2022-11-29.
//

#include "MusicPlayer.h"


MusicPlayer::MusicPlayer() {
    m_filenames["baseGameTheme"] = "../assets/Music/BaseGameTheme.ogg";
    m_filenames["menuTheme"]     = "../assets/Music/MenuTheme.ogg";
    m_filenames["bossTheme"]     = "../assets/Music/BossTheme.ogg";  
}


MusicPlayer &MusicPlayer::getInstance() {
    static MusicPlayer instance;
    return instance;
}


void MusicPlayer::play(String theme) {
    if (!m_music.openFromFile(m_filenames[theme]))
        throw std::runtime_error("Music could not open file");

    m_music.setVolume(m_volume);
    m_music.setLoop(true);
    m_music.play();
}


void MusicPlayer::stop() {
    m_music.stop();
}


void MusicPlayer::setPaused(bool paused) {
    if (paused)
        m_music.pause();
    else
        m_music.play();
}


void MusicPlayer::setVolume(float volume) {
    m_volume = volume;
    m_music.setVolume(m_volume);
}
