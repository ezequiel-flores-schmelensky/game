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

#ifndef SFMLCLASS_SCENE_GAME_H
#define SFMLCLASS_SCENE_GAME_H

#include "Scene.h"

class Scene_Normal : public Scene {

private:
    std::shared_ptr<Entity>		m_player;
    std::string					m_levelPath;

    bool						m_drawTextures{true};
    bool						m_drawAABB{false};
    bool				        m_drawGrid{false};

    void	                    onEnd() override;

    //systems
    void                        sMovement(sf::Time dt);
    void                        sCollisions();
    void                        sUpdate(sf::Time dt);


    // helper functions
    void                        registerActions();
    void                        init(const std::string& configPath);
    void                        loadFromFile(const std::string &configPath);
    void                        spawnPlayer();
    void                        drawAABB();
    void                        keepEntitiesInBounds();
    void                        adjustPlayerPosition();
    sf::FloatRect               getViewBounds();


public:
    Scene_Normal(GameEngine* gameEngine, const std::string& levelPath);

    void		                update(sf::Time dt) override;
    void		                sDoAction(const Action& action) override;
    void		                sRender() override;

};


#endif //SFMLCLASS_SCENE_GAME_H
