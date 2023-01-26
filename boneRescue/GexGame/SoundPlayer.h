//
// Created by David Burchill on 2022-11-29.
//

#ifndef SFMLCLASS_SOUNDPLAYER_H
#define SFMLCLASS_SOUNDPLAYER_H


#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/System/Vector2.hpp>

#include <map>
#include <list>
#include <string>
#include <memory>

using String = std::string;

class SoundPlayer {
protected:
    SoundPlayer();

private:
    std::map<String, std::unique_ptr<sf::SoundBuffer>>  m_soundBuffers;
    std::list<sf::Sound>                                m_sounds;

public:
    static SoundPlayer& getInstance();

    // no copy/move for singelton
    SoundPlayer(const SoundPlayer& ) = delete;
    SoundPlayer(SoundPlayer&&) = delete;
    SoundPlayer& operator=(const SoundPlayer&) = delete;
    SoundPlayer& operator=(SoundPlayer&&) = delete;

public:
    void			    play(String effect);
    void			    play(String effect, sf::Vector2f position);
    void			    removeStoppedSounds();
    void			    setListnerPosition(sf::Vector2f position);
    sf::Vector2f	    getListnerPosition() const;

    bool                isEmpty() const;
    void			    loadBuffer(String id, const String path);
};



#endif //SFMLCLASS_SOUNDPLAYER_H
