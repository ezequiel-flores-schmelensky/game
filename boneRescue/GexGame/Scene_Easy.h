////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
//  Assignment:     Final Project
//  Instructor:     David Burchill
//  File name:      sfml.cpp
// 
//  Student name:   Ezequiel Flores Schmelensky
//  Student email:  ezequielfloressch@gmail.com
// 
//     I certify that this work is my work only, any work copied from Stack Overflow, textbooks, 
//     or elsewhere is properly cited. 
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////

//  BUG
//  list any and all bugs in your code 

#ifndef SFMLCLASS_SCENE_GEXFIGHTER_H
#define SFMLCLASS_SCENE_GEXFIGHTER_H

#include "Scene.h"
#include <map>
#include <array>
#include <bitset>


struct EnemyConfig {
    std::array<float, 5> dirs;
    std::array<sf::Time, 5> times;
};

using String = std::string;

class Scene_Easy : public Scene {

private:
    std::shared_ptr<Entity>		    m_player;

    std::map<String, sf::IntRect>   m_textRects;

    sf::View                        m_worldView;
    sf::FloatRect                   m_worldBounds;
    std::map<std::string, bool>     m_stopers{ {"T", false}, {"R", false}, {"L", false}, {"B", false} }; //top-right-left-bottom

    float                           m_scrollSpeed;
    float                           m_playerSpeed;
    float                           m_enemySpeed{100.f};
    std::map<String, EnemyConfig>   m_enemyConfig;

    float                           m_barkSpeed;
    float                           m_webSpeed;
    sf::Time                        m_fireInterval{ sf::seconds(2) };
    sf::Time                        m_fireDoveInterval{ sf::seconds(7) };
    sf::Time                        m_fireCatInterval{ sf::seconds(5) };
    sf::Time                        m_fireSpiderInterval{ sf::seconds(10) };
    sf::Time                        m_fireBigCatInterval{ sf::seconds(10) };
    sf::Time                        m_frozingInterval{ sf::seconds(2) };
    sf::Time                        m_lionInterval{ sf::seconds(4) };
    int                             m_ballLifeSpan{ 8 };

    sf::Vector2f                    m_spawnPosition;
    sf::Vector2f                    m_spawnPlayerPosition;
    sf::Vector2f                    m_clickPosition;
    bool						    m_drawTextures{true};
    bool						    m_drawAABB{false};
    bool				            m_drawGrid{false};
    bool				            m_finalBoss{false};
    bool				            m_enableScroll{true};
    bool                            m_worldloaded{false};

    void	                        onEnd() override;

    //systems
    void                            sMovement(sf::Time dt);
    void                            sCollisions();
    void                            sUpdate(sf::Time dt);
    void                            sGunUpdate(sf::Time dt);
    void                            sAnimation(sf::Time dt);
    void                            sGuideWeb(sf::Time dt);
    void                            sRemoveEntitiesOutOfGame();

    // helper functions
    sf::Vector2f                    findClosestEnemy(sf::Vector2f mPos);
    void                            createBullet(sf::Vector2f pos, bool isEnemy, std::string animationType, float flipped, bool isLion);
    void                            registerActions();
    void                            init(const std::string& configPath);
    void                            loadFromFile(const std::string &configPath);
    void                            spawnPlayer();
    void                            drawAABB();
    void                            keepEntitiesInBounds();
    void                            adjustScroll(sf::Time& dt);
    void                            adjustPlayer();
    sf::FloatRect                   getViewBounds();
    void                            bark();
    void                            launchWeb();
    void                            droppingAPickup(sf::Vector2f pos, std::string enemyType);
    void                            checkDogCollision();
    void                            checkDogWebCollision();
    void                            checkGunCollision();
    void                            checkPickupCollision();
    void                            checkIfDead(NttPtr e);
    void                            sLifespan(sf::Time dt);

public:
    Scene_Easy(GameEngine* gameEngine, const std::string& configPath);

    void		                    update(sf::Time dt) override;
    void		                    sDoAction(const Action& action) override;
    void		                    sRender() override;


    void playerMovement(sf::Time dt);
    void checkPlayerState();
};


#endif //SFMLCLASS_SCENE_GEXFIGHTER_H
